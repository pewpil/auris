/**
 * Live three.js view of the head-frame placement.
 *
 * The compute unit publishes one JSON snapshot per placement update at
 * `/state`; this module draws it in the head body frame -- +X forward, +Y left,
 * +Z up -- using metres directly, so every length on screen matches the
 * geometry that drove the audio.
 *
 * The scene shows four things:
 *
 *   * the head as a sphere with a nose cone on +X,
 *   * the pointer as a cube whose local +X is the aim axis,
 *   * the ToF beam from the pointer to the hit, whose length is the measured
 *     range,
 *   * the head-to-hit line, which is the direction the sound is placed in.
 *
 * Query parameters:
 *   `server=URL`  snapshot origin; defaults to this page's origin when served
 *                 by the compute unit, otherwise enter the Pi address.
 *   `demo`        animate synthetic data instead of polling a server.
 */

import * as THREE from 'three';
import { OrbitControls } from './vendor/OrbitControls.js';

const X_AXIS = new THREE.Vector3(1, 0, 0);

/** Head sphere radius, metres. */
const HEAD_RADIUS_M = 0.11;

/** Pointer cube dimensions along its body axes, metres. */
const POINTER_LENGTH_M = 0.16;
const POINTER_WIDTH_M = 0.07;
const POINTER_HEIGHT_M = 0.07;

/** Target start-to-start spacing between snapshot requests, ms. */
const POLL_INTERVAL_MS = 20;

/** Request-start history used to report the observed poll cadence, ms. */
const POLL_RATE_WINDOW_MS = 5000;

/** Bound one snapshot request so a stalled route cannot freeze status updates, ms. */
const FETCH_TIMEOUT_MS = 2500;

/** Mark camera capture stalled when its published frame counter stops, ms. */
const CAMERA_FRAME_STALE_MS = 1000;

/** Camera frame history used to smooth the displayed capture-rate estimate, ms. */
const CAMERA_RATE_WINDOW_MS = 5000;

/** Mean 0–255 grayscale luma below which the dashboard warns about dim camera frames. */
const CAMERA_LOW_LIGHT_THRESHOLD = 40;

/** Rolling history used to report observed pointer yaw change, ms. */
const YAW_CHANGE_WINDOW_MS = 60_000;

/** Minimum history span before reporting a normalized yaw-change rate, ms. */
const YAW_CHANGE_MIN_SPAN_MS = 15_000;

/** Empty means use the page origin, which works when the Pi serves this page. */
const DEFAULT_SERVER = '';

/** localStorage key remembering the last server the user typed. */
const STORAGE_KEY = 'auris.viz.server';

const COLORS = {
  background: 0x0b0f14,
  gridMajor: 0x2a4258,
  gridMinor: 0x1a2b3a,
  head: 0x7fb2ff,
  nose: 0xffc857,
  pointer: 0xff9f43,
  pointerEdge: 0xffd7a1,
  beam: 0x38d6ff,
  sight: 0xdfe8f2,
  baseline: 0x7089a3,
  hitNear: 0x4ade80,
  hitFar: 0xf87171,
};

const params = new URLSearchParams(location.search);
const sheetPointer = { startY: null, moved: false, suppressClick: false };

const canvas = document.querySelector('#view');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));

const scene = new THREE.Scene();
scene.background = new THREE.Color(COLORS.background);
scene.fog = new THREE.Fog(COLORS.background, 6, 16);

const camera = new THREE.PerspectiveCamera(50, 1, 0.01, 60);
camera.up.set(0, 0, 1);
camera.position.set(-2.4, -3.2, 2.8);

const controls = new OrbitControls(camera, canvas);
controls.target.set(0.8, 0, 0.35);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.minDistance = 0.4;
controls.maxDistance = 16;
controls.update();

scene.add(new THREE.HemisphereLight(0xbfd4ff, 0x1b2430, 1.1));
const keyLight = new THREE.DirectionalLight(0xffffff, 1.6);
keyLight.position.set(2.4, -2.0, 3.0);
scene.add(keyLight);
const rimLight = new THREE.DirectionalLight(0x6fa8ff, 0.5);
rimLight.position.set(-2.0, 2.4, 1.0);
scene.add(rimLight);

// Ground plane of the head frame: the XY plane, with +Z up.
const grid = new THREE.GridHelper(8, 16, COLORS.gridMajor, COLORS.gridMinor);
grid.rotation.x = Math.PI / 2;
grid.position.z = -0.001;
scene.add(grid);
scene.add(new THREE.AxesHelper(0.6));

const headGroup = new THREE.Group();
const headMaterial = new THREE.MeshStandardMaterial({
  color: COLORS.head, roughness: 0.45, metalness: 0.1,
  transparent: true, opacity: 0.9,
});
headGroup.add(new THREE.Mesh(
  new THREE.SphereGeometry(HEAD_RADIUS_M, 32, 24), headMaterial,
));

// The nose: a cone on the head's +X, i.e. the facing direction.
const nose = new THREE.Mesh(
  new THREE.ConeGeometry(0.035, 0.13, 20),
  new THREE.MeshStandardMaterial({ color: COLORS.nose, roughness: 0.5 }),
);
nose.rotation.z = -Math.PI / 2;
nose.position.set(HEAD_RADIUS_M + 0.055, 0, 0);
headGroup.add(nose);
scene.add(headGroup);

const pointerGroup = new THREE.Group();
const smoothedCameraPosition = new THREE.Vector3();
const smoothedCameraQuaternion = new THREE.Quaternion();
let cameraPoseInitialized = false;
let lastCameraPoseAt = 0;
let cameraFeedUrl = '';
const cameraLumaCanvas = document.createElement('canvas');
cameraLumaCanvas.width = 32;
cameraLumaCanvas.height = 24;
const cameraLumaContext = cameraLumaCanvas.getContext('2d', { willReadFrequently: true });
let lastCameraFrameCount = null;
let lastCameraFrameAt = null;
let estimatedCameraFps = null;
let cameraFramesFresh = null;
/** Recent distinct camera frame-counter samples, each with a monotonic time and count. */
let cameraFrameSamples = [];
/** Pointer yaw samples used only for a display-side change-rate diagnostic. */
let yawChangeSamples = [];
let lastImuYawRaw = null;
let lastImuYawUnwrapped = null;
const pointerMaterial = new THREE.MeshStandardMaterial({
  color: COLORS.pointer, roughness: 0.4, metalness: 0.15,
  transparent: true, opacity: 1,
});
const pointerBox = new THREE.Mesh(
  new THREE.BoxGeometry(POINTER_LENGTH_M, POINTER_WIDTH_M, POINTER_HEIGHT_M),
  pointerMaterial,
);
pointerGroup.add(pointerBox);
const pointerEdgeMaterial = new THREE.LineBasicMaterial({
  color: COLORS.pointerEdge, transparent: true, opacity: 0.9,
});
pointerGroup.add(new THREE.LineSegments(
  new THREE.EdgesGeometry(pointerBox.geometry), pointerEdgeMaterial,
));
const aimTip = new THREE.Mesh(
  new THREE.ConeGeometry(0.018, 0.05, 12),
  new THREE.MeshBasicMaterial({ color: COLORS.pointerEdge }),
);
aimTip.rotation.z = -Math.PI / 2;
aimTip.position.set(POINTER_LENGTH_M / 2 + 0.025, 0, 0);
pointerGroup.add(aimTip);
scene.add(pointerGroup);

const beamGeometry = new THREE.CylinderGeometry(0.007, 0.007, 1, 12, 1, true);
beamGeometry.rotateZ(-Math.PI / 2);
const beamMesh = new THREE.Mesh(
  beamGeometry,
  new THREE.MeshBasicMaterial({
    color: COLORS.beam, transparent: true, opacity: 0.8,
    depthWrite: false, side: THREE.DoubleSide,
  }),
);
scene.add(beamMesh);

const hitMesh = new THREE.Mesh(
  new THREE.SphereGeometry(0.035, 18, 14),
  new THREE.MeshBasicMaterial({ color: COLORS.hitNear }),
);
scene.add(hitMesh);

const sightLine = new THREE.Line(
  new THREE.BufferGeometry().setFromPoints([
    new THREE.Vector3(), new THREE.Vector3(),
  ]),
  new THREE.LineDashedMaterial({
    color: COLORS.sight, dashSize: 0.06, gapSize: 0.04,
    transparent: true, opacity: 0.4,
  }),
);
scene.add(sightLine);

const baselineLine = new THREE.Line(
  new THREE.BufferGeometry().setFromPoints([
    new THREE.Vector3(), new THREE.Vector3(),
  ]),
  new THREE.LineBasicMaterial({
    color: COLORS.baseline, transparent: true, opacity: 0.6,
  }),
);
scene.add(baselineLine);

const rotationMatrix = new THREE.Matrix4();
const hitFar = new THREE.Color(COLORS.hitFar);
const hitNear = new THREE.Color(COLORS.hitNear);
const hitTint = new THREE.Color();

let serverUrl = DEFAULT_SERVER;
let lastState = null;
let fittedOnce = false;
let lastHit = null;
let lastOrigin = new THREE.Vector3();
let viewPaused = false;
let demoActive = false;
let pollInFlight = false;
let pollStartSamples = [];

/**
 * Load a row-major 3x3 rotation into a 4x4 matrix.
 *
 * `Matrix4.set` takes all sixteen elements in row-major order; passing only the
 * nine rotation values would leave the rest undefined and yield a NaN
 * quaternion, which makes the mesh disappear.
 *
 * @param {THREE.Matrix4} matrix Matrix to overwrite.
 * @param {number[]} values Nine rotation values in row-major order.
 */
function setRotationMatrix(matrix, values) {
  matrix.set(
    values[0], values[1], values[2], 0,
    values[3], values[4], values[5], 0,
    values[6], values[7], values[8], 0,
    0, 0, 0, 1,
  );
}

/**
 * Update one two-point line's endpoints.
 *
 * @param {THREE.Line} line Line whose geometry has exactly two vertices.
 * @param {THREE.Vector3} from Start point in the head frame, metres.
 * @param {THREE.Vector3} to End point in the head frame, metres.
 */
function setLine(line, from, to) {
  const position = line.geometry.getAttribute('position');
  position.setXYZ(0, from.x, from.y, from.z);
  position.setXYZ(1, to.x, to.y, to.z);
  position.needsUpdate = true;
  line.geometry.computeBoundingSphere();
  if (line.material.isLineDashedMaterial) {
    line.computeLineDistances();
  }
}

/**
 * Map a gain onto the hit marker's colour.
 *
 * @param {number|null} gain Amplitude multiplier in 0..1.
 * @returns {THREE.Color} Green at full gain, red at silence.
 */
function hitColour(gain) {
  const value = Number.isFinite(gain) ? THREE.MathUtils.clamp(gain, 0, 1) : 0;
  return hitTint.copy(hitFar).lerp(hitNear, value);
}

/**
 * Draw one snapshot.
 *
 * Missing pieces hide rather than throw: a page that has not received data yet
 * still renders the head and the grid.
 *
 * The pointer cube sits at the camera-observed absolute position when available,
 * then falls back to the inertial estimate and audio baseline. The beam and hit
 * follow that origin whenever a usable pointer orientation and range exist.
 * With no head-registered pose, a fresh pointer IMU can drive a pointer-local view;
 * its display origin is arbitrary and never claims a head-relative position.
 *
 * @param {object} state Snapshot from the compute unit's `/state`.
 */
function applyState(state) {
  lastState = state;
  const placement = state.placement ?? null;
  const pointerState = state.pointer ?? null;
  const trackedState = state.tracked ?? null;
  const cameraState = state.camera ?? null;
  const rawCameraPosition = cameraState && Array.isArray(cameraState.position_head_m)
    && cameraState.position_head_m.length === 3
    ? new THREE.Vector3().fromArray(cameraState.position_head_m) : null;
  const cameraPositionFresh = Boolean(rawCameraPosition && cameraState.position_fresh === true);
  const cameraRotation = cameraState && Array.isArray(cameraState.rotation_head)
    && cameraState.rotation_head.length === 9 ? cameraState.rotation_head : null;
  const pointerInertialRotation = pointerState
    && Array.isArray(pointerState.matrix_pointer_inertial)
    && pointerState.matrix_pointer_inertial.length === 9
    ? pointerState.matrix_pointer_inertial : null;
  const pointerInertialFresh = Boolean(pointerInertialRotation
    && pointerState?.imu_fresh === true
    && (!Number.isFinite(pointerState.sample_age_s)
      || pointerState.sample_age_s <= 0.25));
  let cameraPosition = rawCameraPosition;
  let smoothedCameraRotation = null;
  if (cameraPositionFresh && cameraRotation) {
    const matrix = new THREE.Matrix4();
    setRotationMatrix(matrix, cameraRotation);
    const targetRotation = new THREE.Quaternion().setFromRotationMatrix(matrix);
    const now = performance.now();
    const alpha = cameraPoseInitialized
      ? 1 - Math.exp(-Math.max(0, now - lastCameraPoseAt) / 60)
      : 1;
    if (!cameraPoseInitialized) {
      smoothedCameraPosition.copy(rawCameraPosition);
      smoothedCameraQuaternion.copy(targetRotation);
      cameraPoseInitialized = true;
    } else {
      smoothedCameraPosition.lerp(rawCameraPosition, alpha);
      smoothedCameraQuaternion.slerp(targetRotation, alpha);
    }
    lastCameraPoseAt = now;
    cameraPosition = smoothedCameraPosition;
    smoothedCameraRotation = smoothedCameraQuaternion;
  } else if (cameraState?.position_fresh === false) {
    cameraPoseInitialized = false;
  }
  const trackedPosition = trackedState && Array.isArray(trackedState.origin_m)
    ? new THREE.Vector3().fromArray(trackedState.origin_m) : null;
  const placementPosition = placement && Array.isArray(placement.origin_m)
    ? new THREE.Vector3().fromArray(placement.origin_m) : null;
  const headImuFresh = state.head?.fresh === true
    && Array.isArray(state.head.euler_deg);
  const headImuSeen = Number.isFinite(state.head?.age_s);
  const pointerHeadRotation = pointerState && Array.isArray(pointerState.matrix_head)
    && pointerState.matrix_head.length === 9 ? pointerState.matrix_head : null;
  const pointerOnlyFrame = Boolean(pointerInertialFresh && !cameraPositionFresh
    && !trackedPosition && !headImuSeen);
  const coordinateNote = document.getElementById('frame-coordinate-note');
  if (coordinateNote) {
    coordinateNote.textContent = pointerOnlyFrame
      ? 'Pointer AHRS filter-world frame · yaw unregistered'
      : 'Head frame · +X forward · +Y left · +Z up';
  }
  const origin = pointerOnlyFrame ? new THREE.Vector3()
    : (cameraPositionFresh ? cameraPosition : null)
      ?? trackedPosition ?? placementPosition ?? cameraPosition
      ?? new THREE.Vector3();
  const usable = Boolean(
    placement && placement.usable && placement.beam
    && Number.isFinite(placement.range_m),
  );

  const pointerVisible = Boolean(pointerOnlyFrame || cameraPosition || trackedPosition
    || pointerHeadRotation);
  headGroup.visible = !pointerOnlyFrame;
  pointerGroup.visible = pointerVisible;
  pointerGroup.position.copy(origin);
  lastOrigin.copy(origin);
  pointerMaterial.opacity = pointerOnlyFrame ? 0.9
    : usable ? 1 : cameraPositionFresh ? 0.85 : cameraPosition ? 0.35 : 0.35;
  pointerEdgeMaterial.opacity = pointerOnlyFrame ? 0.95
    : usable ? 0.9 : cameraPositionFresh ? 0.8 : cameraPosition ? 0.35 : 0.3;
  if (pointerVisible) {
    if (cameraPositionFresh && smoothedCameraRotation) {
      pointerGroup.quaternion.copy(smoothedCameraRotation);
    } else {
      const orientation = pointerOnlyFrame ? pointerInertialRotation
        : pointerHeadRotation ?? cameraRotation;
      if (orientation) {
        setRotationMatrix(rotationMatrix, orientation);
        pointerGroup.quaternion.setFromRotationMatrix(rotationMatrix);
      } else {
        pointerGroup.quaternion.identity();
      }
    }
  }

  const baselineLength = origin.length();
  baselineLine.visible = pointerVisible && baselineLength > 1e-3;
  if (baselineLine.visible) {
    setLine(baselineLine, new THREE.Vector3(), origin);
  }

  if (!usable) {
    sightLine.visible = false;
    const localRange = pointerOnlyFrame && pointerState?.tof_fresh === true
      && Number.isFinite(pointerState.range_m) && pointerState.range_m > 0
      ? pointerState.range_m : null;
    if (localRange !== null) {
      const localBeam = new THREE.Vector3(1, 0, 0)
        .applyQuaternion(pointerGroup.quaternion).normalize();
      const localHit = origin.clone().addScaledVector(localBeam, localRange);
      beamMesh.visible = true;
      beamMesh.position.copy(origin).addScaledVector(localBeam, localRange / 2);
      beamMesh.quaternion.setFromUnitVectors(X_AXIS, localBeam);
      beamMesh.scale.set(localRange, 1, 1);
      hitMesh.visible = true;
      hitMesh.position.copy(localHit);
      hitMesh.material.color.setHex(COLORS.nose);
      lastHit = localHit;
    } else {
      beamMesh.visible = false;
      hitMesh.visible = false;
    }
    const frameNote = document.getElementById('frame-note');
    if (frameNote) {
      frameNote.textContent = pointerOnlyFrame
        ? 'Pointer IMU frame only · yaw is unregistered; origin is a display anchor, not a head-relative position. The ToF endpoint is relative to the pointer.'
        : cameraPosition && !cameraPositionFresh
          ? headImuFresh
            ? 'Head frame · camera pose is held; orientation uses the fresh head IMU.'
            : 'Head frame · camera pose and head IMU reference are held stale.'
          : headImuFresh
            ? 'Head frame · orientation comes from the head IMU; pointer position needs a camera fix or inertial estimate.'
            : headImuSeen
              ? 'Head frame · using the last head reference; its IMU sample is stale.'
              : 'No shared head frame · waiting for an ArUco pose or fresh head IMU sample.';
    }
    return;
  }

  const beam = new THREE.Vector3().fromArray(placement.beam).normalize();
  const length = placement.range_m;
  const hitPoint = origin.clone().addScaledVector(beam, length);
  lastHit = hitPoint.clone();

  beamMesh.visible = length > 1e-4;
  beamMesh.position.copy(origin).addScaledVector(beam, length / 2);
  beamMesh.quaternion.setFromUnitVectors(X_AXIS, beam);
  beamMesh.scale.set(length, 1, 1);

  hitMesh.visible = true;
  hitMesh.position.copy(hitPoint);
  hitMesh.material.color.copy(hitColour(placement.gain));

  sightLine.visible = true;
  setLine(sightLine, new THREE.Vector3(), hitPoint);
  const frameNote = document.getElementById('frame-note');
  if (frameNote) {
    frameNote.textContent = cameraPositionFresh
      ? 'Head frame · fresh ArUco cube pose supplies raygun position and orientation.'
      : headImuFresh
        ? 'Head frame · pointer pose uses the head IMU with the latest visual yaw alignment.'
        : 'Head frame · pose source is shown in the sensor panel.';
  }

  if (!fittedOnce) {
    lastHit = hitPoint.clone();
    fittedOnce = true;
    fitView();
  }
}

/**
 * Frame the head and the current hit in the existing viewing direction.
 *
 * The default camera is a compromise between near and far targets; this puts
 * both ends of the current geometry in view without changing which side the
 * scene is seen from. Bound to the `f` key and run once on the first hit.
 */
function fitView() {
  if (!lastHit) return;

  const points = [new THREE.Vector3(), lastHit.clone(), lastOrigin.clone()];
  const sphere = new THREE.Box3().setFromPoints(points)
    .getBoundingSphere(new THREE.Sphere());
  const direction = new THREE.Vector3()
    .subVectors(camera.position, controls.target)
    .normalize();
  const halfFov = THREE.MathUtils.degToRad(camera.fov / 2);
  const distance = Math.max(1.2, (sphere.radius / Math.sin(halfFov)) * 1.35);

  controls.target.copy(sphere.center);
  camera.position.copy(sphere.center).addScaledVector(direction, distance);
  controls.update();
}

/**
 * Format a number for the panel.
 *
 * @param {number|null|undefined} value Number to show.
 * @param {number} [digits] Decimal places.
 * @returns {string} The number, or an em dash when it is not finite.
 */
function fmt(value, digits = 2) {
  return Number.isFinite(value) ? value.toFixed(digits) : '—';
}

/**
 * Format an angle with an explicit sign.
 *
 * @param {number|null|undefined} value Angle in degrees.
 * @param {number} [digits] Decimal places.
 * @returns {string} Signed degrees, or an em dash when it is not finite.
 */
function signed(value, digits = 1) {
  if (!Number.isFinite(value)) return '—';
  return `${value >= 0 ? '+' : ''}${value.toFixed(digits)}°`;
}

/**
 * Format the normalized stereo balance for a left-to-right readout.
 *
 * @param {number|null|undefined} pan Unitless pan in -1..1; -1 is full left and +1 is full right.
 * @returns {string} Side and percentage, `Center`, or an em dash for invalid data.
 */
function formatPan(pan) {
  if (!Number.isFinite(pan)) return '—';
  const value = THREE.MathUtils.clamp(pan, -1, 1);
  if (Math.abs(value) < 0.005) return 'Center';
  return `${value < 0 ? 'L' : 'R'} ${Math.round(Math.abs(value) * 100)}%`;
}

/**
 * Write one HUD value.
 *
 * @param {string} id Element id.
 * @param {string} text Text to show.
 * @param {string} [className] Optional colour class.
 */
function setText(id, text, className = 'metric-value') {
  const element = document.getElementById(id);
  if (!element) return;
  element.textContent = text;
  element.className = className;
}

/**
 * Scale a distance bar against the configured maximum range.
 *
 * @param {string} id Meter container id.
 * @param {number|null|undefined} distanceM Current distance in metres.
 * @param {number|null|undefined} maximumM Maximum range in metres.
 * @param {string} maximumLabelId Label id for the displayed scale maximum.
 */
function setDistanceMeter(id, distanceM, maximumM, maximumLabelId) {
  const meter = document.getElementById(id);
  const label = document.getElementById(maximumLabelId);
  if (!meter) return;
  const valid = Number.isFinite(distanceM) && distanceM >= 0
    && Number.isFinite(maximumM) && maximumM > 0;
  const safeDistance = valid ? distanceM : 0;
  const safeMaximum = Number.isFinite(maximumM) && maximumM > 0 ? maximumM : 0;
  const fill = meter.querySelector('.range-meter-fill');
  if (fill) fill.style.width = `${valid ? Math.min(100, safeDistance / safeMaximum * 100) : 0}%`;
  meter.setAttribute('aria-valuemax', String(safeMaximum));
  meter.setAttribute('aria-valuenow', String(safeDistance));
  if (label) label.textContent = `${safeMaximum.toFixed(1)} m max`;
}

/**
 * Draw a normalized stereo pan from a fixed centre mark toward the active side.
 *
 * @param {string} id Meter element id.
 * @param {number|null|undefined} pan Unitless pan in -1..1; -1 is full left and +1 is full right.
 * @returns {void} Update the visual fill and accessible value text.
 */
function setPanMeter(id, pan) {
  const meter = document.getElementById(id);
  if (!meter) return;
  const valid = Number.isFinite(pan);
  const value = valid ? THREE.MathUtils.clamp(pan, -1, 1) : 0;
  const fill = meter.querySelector('.pan-meter-fill');
  if (fill) {
    fill.style.left = `${50 + Math.min(0, value) * 50}%`;
    fill.style.width = `${Math.abs(value) * 50}%`;
  }
  meter.setAttribute('aria-valuenow', String(value));
  meter.setAttribute('aria-valuetext', valid ? formatPan(value) : 'Stereo pan unavailable');
}

/**
 * Update every readout in the panel.
 *
 * @param {object} state Snapshot from the compute unit's `/state`.
 */
function updateHud(state) {
  const head = state.head ?? {};
  const placement = state.placement ?? null;
  const pointer = state.pointer ?? null;
  const cameraFix = state.camera ?? {};
  const cameraMarkerIds = Array.isArray(cameraFix.marker_ids) ? cameraFix.marker_ids : [];
  const maxRange = state.config?.max_range_m;
  const euler = head.euler_deg;
  const pointerSource = typeof pointer?.source === 'string'
    ? pointer.source : typeof state.source === 'string' ? state.source : null;

  setText('st-roll', euler ? signed(euler[0]) : '—');
  setText('st-pitch', euler ? signed(euler[1]) : '—');
  setText('st-yaw', euler ? signed(euler[2]) : '—');
  const headSource = typeof head.source === 'string' ? head.source : null;
  const headAge = Number.isFinite(head.age_s)
    ? `${Math.max(0, head.age_s * 1000).toFixed(0)} ms` : null;
  const headState = head.fresh === true
    ? `${headSource ?? 'IMU'} · fresh${headAge ? ` · ${headAge}` : ''}`
    : headSource === 'none' ? 'disabled · source none'
      : Array.isArray(euler) ? `${headSource ?? 'IMU'} · stale`
        : headSource ? `${headSource} · no sample` : 'no data';
  setText('st-head-health', headState,
    head.fresh === true ? 'metric-value good' : 'metric-value warn');
  const headInputDiagnosis = head.fresh === true
    ? `${headSource ?? 'Head IMU'} stream is supplying fresh samples.`
    : headSource === 'none' && pointerSource === 'usb-table'
      ? 'The active USB stream is the pointer debug table; it carries no head IMU (H1) records.'
      : headSource === 'none'
        ? 'Head IMU input is disabled in the compute-unit pipeline.'
        : headSource
          ? `No fresh samples are arriving from the ${headSource} head input.`
          : 'The compute unit has not reported a head IMU input.';
  setText('st-head-diagnosis', headInputDiagnosis,
    head.fresh === true ? 'metric-value good wrap' : 'metric-value warn wrap');

  const cameraRotation = cameraFix.rotation_head;
  const cameraOrientationFresh = cameraFix.position_fresh === true
    && Array.isArray(cameraRotation) && cameraRotation.length === 9;
  const pointerInertialRotation = Array.isArray(pointer?.matrix_pointer_inertial)
    && pointer.matrix_pointer_inertial.length === 9 ? pointer.matrix_pointer_inertial : null;
  const pointerInertialFresh = Boolean(pointerInertialRotation
    && pointer?.imu_fresh === true
    && (!Number.isFinite(pointer?.sample_age_s) || pointer.sample_age_s <= 0.25));
  const pointerHeadAvailable = Boolean(pointer?.matrix_head
    && head && Number.isFinite(head.age_s));
  const orientationMatrix = cameraOrientationFresh
    ? cameraRotation : pointerHeadAvailable ? pointer.matrix_head
      : pointerInertialFresh ? pointerInertialRotation : cameraRotation;
  const pointerAngles = orientationMatrix
    ? orientationDegrees(orientationMatrix) : null;
  setText('st-pointer-roll', pointerAngles ? signed(pointerAngles[0]) : '—');
  setText('st-pointer-pitch', pointerAngles ? signed(pointerAngles[1]) : '—');
  setText('st-pointer-yaw', pointerAngles ? signed(pointerAngles[2]) : '—');
  setText('st-orientation-source', cameraOrientationFresh
    ? `ArUco cube · ID ${cameraMarkerIds.join(',') || '—'}`
    : pointerHeadAvailable ? (head.fresh === true
      ? 'pointer IMU · head frame' : 'pointer IMU · held head frame')
      : pointerInertialFresh ? 'pointer IMU · local frame, yaw unregistered'
        : Array.isArray(cameraRotation) ? 'last camera pose' : 'unavailable');
  updateYawChange(pointerHeadAvailable ? pointer.matrix_head
    : pointerInertialFresh ? pointerInertialRotation : null,
  cameraOrientationFresh, state.mock === true);
  const pointerSourceLabel = pointerSource === 'usb-table'
    ? 'USB debug table' : pointerSource;
  const pointerSchemaLabel = typeof pointer?.schema === 'string'
    && pointer.schema !== pointerSource ? `row ${pointer.schema}` : null;
  const pointerAge = Number.isFinite(pointer?.sample_age_s)
    ? Math.max(0, pointer.sample_age_s) : null;
  const pointerDetails = [];
  if (pointer?.imu_fresh === true) pointerDetails.push('IMU fresh');
  else if (pointer?.imu_fresh === false) pointerDetails.push('IMU stale');
  if (pointer?.tof_fresh === true) pointerDetails.push('ToF fresh');
  else if (pointer?.tof_fresh === false) pointerDetails.push('ToF stale');
  if (pointerAge !== null) pointerDetails.push(`${(pointerAge * 1000).toFixed(0)} ms old`);
  const pointerStream = state.mock === true ? 'synthetic demo'
    : pointerSourceLabel ? [pointerSourceLabel, pointerSchemaLabel, ...pointerDetails]
      .filter(Boolean).join(' · ')
      : pointer ? ['source not reported', ...pointerDetails].join(' · ')
        : 'no pointer data';
  const pointerStreamHealthy = pointerSource !== null
    && pointer?.imu_fresh === true && pointer?.tof_fresh === true
    && pointerAge !== null && pointerAge <= 0.25;
  const pointerStreamClass = state.mock === true ? 'metric-value info'
    : pointerStreamHealthy ? 'metric-value good' : 'metric-value warn';
  setText('st-pointer-stream', pointerStream, pointerStreamClass);
  const pointerCalibration = pointer?.calibrated === true ? 'calibrated'
    : pointer?.calibrated === false ? 'uncalibrated' : 'not reported';
  const pointerFlags = Number.isInteger(pointer?.flags)
    ? ` · flags 0x${pointer.flags.toString(16).padStart(2, '0')}` : '';
  setText('st-pointer-cal', pointerCalibration + pointerFlags,
    pointer?.calibrated === true ? 'metric-value good'
      : pointer?.calibrated === false ? 'metric-value warn' : 'metric-value');
  const accel = pointer?.accel_g;
  const accelText = Array.isArray(accel) && accel.length === 3
    && accel.every(Number.isFinite)
    ? `${accel.map((axis) => axis.toFixed(2)).join(' / ')} g`
    : pointer ? 'not provided by stream' : '—';
  setText('st-accel', accelText);
  const gyro = pointer?.gyro_dps;
  const gyroText = Array.isArray(gyro) && gyro.length === 3
    && gyro.every(Number.isFinite)
    ? `${gyro.map((axis) => axis.toFixed(1)).join(' / ')} °/s`
    : pointerSource === 'usb-table' ? 'not included in USB debug table'
      : pointer ? 'not provided by stream' : '—';
  setText('st-gyro', gyroText);
  const rangeDetails = [];
  if (Number.isInteger(pointer?.range_status)) {
    rangeDetails.push(`status ${pointer.range_status}`);
  }
  if (Number.isFinite(pointer?.range_age_ms)) {
    rangeDetails.push(`${pointer.range_age_ms.toFixed(0)} ms old`);
  }
  setText('st-tof-quality', rangeDetails.length
    ? rangeDetails.join(' · ') : pointer ? 'not provided by stream' : '—');
  setText('st-imu-temp', Number.isFinite(pointer?.temperature_c)
    ? `${pointer.temperature_c.toFixed(1)} °C` : pointer ? 'not provided by stream' : '—');
  setText('st-range', pointer && Number.isFinite(pointer.range_m)
    ? `${pointer.range_m.toFixed(3)} m` : '—',
    pointer && Number.isFinite(pointer.range_m) ? 'metric-value good' : 'metric-value warn');
  setDistanceMeter('tof-meter', pointer?.range_m, maxRange, 'tof-meter-max');
  const rangeSummary = pointer && Number.isFinite(pointer.range_m)
    ? `${pointer.range_m.toFixed(2)} m` : 'ToF —';
  const fetchSummary = document.getElementById('st-fetch')?.textContent ?? '—';
  const summary = document.getElementById('sheet-summary');
  if (summary) summary.textContent = `ToF ${rangeSummary} · ${fetchSummary}`;
  const buttonKnown = pointer && pointer.button !== null
    && pointer.button !== undefined;
  const buttonPressed = buttonKnown
    && (pointer.button === true || pointer.button === 1);
  setText('st-btn', buttonKnown ? (buttonPressed ? 'pressed' : 'released') : '—',
    buttonPressed ? 'metric-value info' : 'metric-value');
  setText('st-beam', placement
    ? `${signed(placement.azimuth_deg)} / ${signed(placement.elevation_deg)}`
    : '—');
  setText('st-dist', placement && Number.isFinite(placement.distance_m)
    ? `${placement.distance_m.toFixed(2)} m` : '—');
  setDistanceMeter('hit-meter', placement?.distance_m, maxRange, 'hit-meter-max');
  setText('st-gain', placement ? fmt(placement.gain, 3) : '—');
  setText('st-pan', placement ? formatPan(placement.pan) : '—');
  setPanMeter('pan-meter', placement?.pan);

  const baseline = placement ? placement.baseline : (state.aligned ? 'waiting' : 'unaligned');
  const baselineClass = baseline === 'camera' ? 'metric-value good'
    : baseline === 'unaligned' || baseline === 'waiting' ? 'metric-value warn' : 'metric-value';
  setText('st-base', baseline, baselineClass);
  setText('st-baseline-health', baseline === 'camera' ? 'marker tracked'
    : baseline === 'hold' ? 'holding last fix'
      : baseline === 'unaligned' || baseline === 'waiting' ? 'waiting for data' : 'assumed origin',
    baseline === 'camera' ? 'metric-value good' : 'metric-value warn');

  const cameraPosition = cameraFix.position_head_m;
  const markerIds = Array.isArray(cameraFix.marker_ids) ? cameraFix.marker_ids : [];
  setText('st-cam', Array.isArray(cameraPosition) && cameraPosition.length === 3
    ? `${Number(cameraPosition[0]).toFixed(3)}, ${Number(cameraPosition[1]).toFixed(3)}, ${Number(cameraPosition[2]).toFixed(3)} m · ID ${markerIds.join(',') || '—'}`
    : Number.isFinite(cameraFix.distance_m)
      ? `distance ${cameraFix.distance_m.toFixed(3)} m · ${fmt(cameraFix.age_s, 2)} s`
      : 'not detected',
  Array.isArray(cameraPosition) && cameraFix.position_fresh === true
    ? 'metric-value good' : 'metric-value warn');
  const cameraFrames = Number.isFinite(cameraFix.frame_count) ? cameraFix.frame_count : null;
  const detections = Number.isFinite(cameraFix.detection_count) ? cameraFix.detection_count : null;
  const frameAgeSeconds = lastCameraFrameAt === null
    ? null : Math.max(0, (performance.now() - lastCameraFrameAt) / 1000);
  const frameRate = cameraFramesFresh === false
    ? `stalled · ${fmt(frameAgeSeconds, 1)} s ago`
    : estimatedCameraFps === null ? 'sampling' : `${estimatedCameraFps.toFixed(1)} fps`;
  setText('st-camera-frames', cameraFrames === null ? 'unavailable'
    : `${cameraFrames} · ${frameRate}`,
  cameraFramesFresh === false ? 'metric-value warn' : 'metric-value');
  setText('st-camera-quality', (detections ?? '—') + ' hits · '
    + fmt(cameraFix.age_s, 3) + ' s · '
    + fmt(cameraFix.reprojection_error_px, 1) + ' px');
  setText('st-camera-time', fmt(cameraFix.read_ms, 1) + ' / '
    + fmt(cameraFix.detection_ms, 1) + ' ms');
  const targetMarkerIds = Array.isArray(cameraFix.target_marker_ids)
    ? cameraFix.target_marker_ids.filter(Number.isInteger) : [];
  const markerConfig = [];
  if (typeof cameraFix.dictionary_name === 'string') {
    markerConfig.push(cameraFix.dictionary_name);
  }
  if (targetMarkerIds.length) markerConfig.push(`IDs ${targetMarkerIds.join(',')}`);
  if (Number.isFinite(cameraFix.marker_size_m)) {
    markerConfig.push(`tag ${(cameraFix.marker_size_m * 1000).toFixed(0)} mm`);
  }
  if (Number.isFinite(cameraFix.cube_size_m)) {
    markerConfig.push(`cube ${(cameraFix.cube_size_m * 1000).toFixed(0)} mm`);
  }
  setText('st-marker-config', markerConfig.length
    ? markerConfig.join(' · ') : 'camera not configured');
  const frameSize = Array.isArray(cameraFix.frame_size_px) ? cameraFix.frame_size_px.join('×') : 'size n/a';
  const calibration = cameraFix.calibration_source || 'calibration n/a';
  const readFailures = Number.isFinite(cameraFix.read_failures) ? cameraFix.read_failures : '—';
  setText('st-camera-calibration', calibration + ' · ' + frameSize + ' · '
    + readFailures + ' failures',
    calibration === 'assumed-fov' ? 'metric-value warn' : 'metric-value');
  setText('st-audio', state.muted ? 'muted' : 'active',
    state.muted ? 'metric-value warn' : 'metric-value good');

  const tracked = state.tracked ?? null;
  if (tracked && tracked.origin_m) {
    const radius = Math.hypot(tracked.origin_m[0], tracked.origin_m[1],
      tracked.origin_m[2]);
    const flags = tracked.pointer_stale ? ' · stale'
      : tracked.holding ? ' · hold'
      : tracked.clamped ? ' · clamp' : '';
    setText('st-track', `${radius.toFixed(2)} m${flags}`,
      tracked.pointer_stale || tracked.clamped ? 'metric-value warn' : 'metric-value');
  } else {
    setText('st-track', '—');
  }

  if (state.aligned === false && Array.isArray(euler)) {
    setText('st-head-health', 'align first', 'metric-value warn');
  }
}

/**
 * Attach the camera's annotated MJPEG stream and report its latest tracking health.
 *
 * @param {string} base Dashboard origin, including its port.
 * @param {object} camera Current camera fields from the Pi snapshot.
 */
function setCameraFeed(base, camera) {
  const image = document.querySelector('#camera-feed');
  const status = document.querySelector('#camera-feed-status');
  const state = document.querySelector('#camera-feed-state');
  const tagHint = document.querySelector('#camera-tag-hint');
  const nextUrl = `${base.replace(/\/$/, '')}/camera`;
  if (image && cameraFeedUrl !== nextUrl) {
    cameraFeedUrl = nextUrl;
    image.src = nextUrl;
  }
  const fresh = camera.position_fresh === true;
  const hasFrames = Number.isFinite(camera.frame_count) && camera.frame_count > 0;
  const markerIds = Array.isArray(camera.marker_ids) ? camera.marker_ids : [];
  const targetMarkerIds = Array.isArray(camera.target_marker_ids)
    ? camera.target_marker_ids.filter(Number.isInteger) : [];
  const tagSizeMm = Number.isFinite(camera.marker_size_m)
    ? `${(camera.marker_size_m * 1000).toFixed(0)} mm` : 'printed';
  if (tagHint) {
    tagHint.hidden = !hasFrames || cameraFramesFresh === false || fresh;
    const targetLabel = targetMarkerIds.length
      ? ` (ID ${targetMarkerIds.join(', ')})` : '';
    tagHint.textContent = markerIds.length
      ? `Last accepted tag ID ${markerIds.join(', ')} is stale. Aim its ${tagSizeMm} face at the camera until the tag locks again.`
      : `No cube tag detected. Aim a ${tagSizeMm} face${targetLabel} at the head camera and move the cube closer.`;
  }
  if (state) {
    const label = cameraFramesFresh === false ? 'stalled'
      : fresh ? 'tag locked' : hasFrames ? 'searching' : 'no frames';
    state.textContent = label;
    state.dataset.level = cameraFramesFresh === false || !hasFrames ? 'bad'
      : fresh ? 'good' : 'warn';
    state.title = cameraFramesFresh === false
      ? 'The camera frame counter has not advanced for at least one second.'
      : fresh ? 'A camera pose is currently available.'
        : hasFrames ? 'Camera frames are arriving; no fresh tag pose is available.'
          : 'No camera frames have been reported.';
  }
  if (status) {
    const ids = markerIds.length ? `IDs ${markerIds.join(',')}` : 'no tags';
    const frames = Number.isFinite(camera.frame_count) ? ` · frame ${camera.frame_count}` : '';
    const age = cameraFramesFresh === false && lastCameraFrameAt !== null
      ? ` · last frame ${fmt((performance.now() - lastCameraFrameAt) / 1000, 1)} s ago`
      : Number.isFinite(camera.age_s) ? ` · pose ${Math.round(camera.age_s * 1000)} ms old` : '';
    status.textContent = `${ids}${frames}${age}`;
  }
}

/**
 * Show the mean grayscale brightness of a downsampled live camera frame.
 *
 * Read the current MJPEG image locally in this browser at 2 Hz. The 0–255
 * value is a rough lighting diagnostic, not a camera calibration metric.
 * Means below CAMERA_LOW_LIGHT_THRESHOLD (0–255 luma) prompt a lighting check;
 * this operator hint does not predict whether ArUco detection will succeed.
 *
 * @returns {void} Update the camera card or report that pixel access is unavailable.
 * @remarks Catch canvas and browser security errors locally and show them as unavailable.
 */
function sampleCameraBrightness() {
  const image = document.getElementById('camera-feed');
  const label = document.getElementById('camera-feed-quality');
  const meter = document.getElementById('camera-luma-meter');
  const hint = document.getElementById('camera-feed-hint');
  if (!label) return;
  if (document.hidden) return;
  if (!image || !cameraLumaContext || image.naturalWidth === 0) {
    label.textContent = 'Image brightness waiting';
    label.dataset.level = 'unavailable';
    if (meter) {
      meter.dataset.level = 'unavailable';
      meter.setAttribute('aria-valuenow', '0');
      meter.setAttribute('aria-valuetext', 'Image brightness unavailable');
      meter.firstElementChild?.style.setProperty('width', '0%');
    }
    if (hint) hint.hidden = true;
    return;
  }
  if (cameraFramesFresh === false) {
    label.textContent = 'Camera frame stalled · brightness is stale';
    label.dataset.level = 'unavailable';
    if (meter) {
      meter.dataset.level = 'unavailable';
      meter.setAttribute('aria-valuenow', '0');
      meter.setAttribute('aria-valuetext', 'Camera frame stalled; brightness is stale');
      meter.firstElementChild?.style.setProperty('width', '0%');
    }
    if (hint) hint.hidden = true;
    return;
  }
  try {
    cameraLumaContext.drawImage(image, 0, 0, cameraLumaCanvas.width, cameraLumaCanvas.height);
    const pixels = cameraLumaContext.getImageData(
      0, 0, cameraLumaCanvas.width, cameraLumaCanvas.height,
    ).data;
    let totalLuma = 0;
    let pixelCount = 0;
    for (let index = 0; index < pixels.length; index += 4) {
      totalLuma += 0.2126 * pixels[index]
        + 0.7152 * pixels[index + 1]
        + 0.0722 * pixels[index + 2];
      pixelCount += 1;
    }
    const meanLuma = pixelCount ? totalLuma / pixelCount : 0;
    const lowLight = meanLuma < CAMERA_LOW_LIGHT_THRESHOLD;
    label.textContent = `${lowLight ? 'Low brightness' : 'Image brightness'} · ${meanLuma.toFixed(1)} / 255`;
    label.dataset.level = lowLight ? 'dark' : 'normal';
    if (meter) {
      meter.dataset.level = lowLight ? 'dark' : 'normal';
      meter.setAttribute('aria-valuenow', meanLuma.toFixed(1));
      meter.setAttribute('aria-valuetext', `Approximate image brightness ${meanLuma.toFixed(1)} of 255`);
      meter.firstElementChild?.style.setProperty('width', `${Math.min(100, meanLuma / 255 * 100)}%`);
    }
    if (hint) hint.hidden = !lowLight;
  } catch {
    label.textContent = 'Image brightness unavailable';
    label.dataset.level = 'unavailable';
    if (meter) {
      meter.dataset.level = 'unavailable';
      meter.setAttribute('aria-valuenow', '0');
      meter.setAttribute('aria-valuetext', 'Image brightness unavailable');
      meter.firstElementChild?.style.setProperty('width', '0%');
    }
    if (hint) hint.hidden = true;
  }
}

/**
 * Convert a row-major pointer rotation into roll, pitch and yaw angles.
 *
 * @param {number[]} values Nine row-major rotation values mapping pointer axes
 *     into the reference frame represented by the matrix.
 * @returns {number[]|null} Roll, pitch and yaw in degrees, or `null` for
 *     invalid data. Angles use the matrix destination frame; callers must label
 *     whether that frame is head-registered or pointer-inertial.
 */
function orientationDegrees(values) {
  if (!Array.isArray(values) || values.length !== 9
      || !values.every(Number.isFinite)) return null;
  setRotationMatrix(rotationMatrix, values);
  const angles = new THREE.Euler().setFromRotationMatrix(rotationMatrix, 'ZYX');
  return [
    THREE.MathUtils.radToDeg(angles.x),
    THREE.MathUtils.radToDeg(angles.y),
    THREE.MathUtils.radToDeg(angles.z),
  ];
}

/**
 * Show observed pointer yaw change without feeding it back into tracking.
 *
 * @param {number[]|null|undefined} pointerMatrix Row-major pointer-to-reference
 *     rotation matrix, unitless, or absent when the pointer stream has no pose.
 * @param {boolean} cameraOrientationFresh Whether a fresh ArUco cube pose is
 *     authoritative for the displayed orientation.
 * @param {boolean} mock Whether the current snapshot contains synthetic data.
 * @returns {void} Update the HUD in degrees per minute or show its source state.
 * @remarks The rate includes physical turns and inertial drift; it is not a
 *     direct gyro-bias estimate. The reference may be the head frame or the
 *     pointer's own unregistered inertial frame. Missing matrices display as unavailable.
 */
function updateYawChange(pointerMatrix, cameraOrientationFresh, mock) {
  if (cameraOrientationFresh) {
    yawChangeSamples = [];
    lastImuYawRaw = null;
    lastImuYawUnwrapped = null;
    setText('st-yaw-trend', 'ArUco locked', 'metric-value good');
    return;
  }
  if (mock) {
    yawChangeSamples = [];
    lastImuYawRaw = null;
    lastImuYawUnwrapped = null;
    setText('st-yaw-trend', 'demo data', 'metric-value info');
    return;
  }

  const angles = orientationDegrees(pointerMatrix);
  if (!angles) {
    yawChangeSamples = [];
    lastImuYawRaw = null;
    lastImuYawUnwrapped = null;
    setText('st-yaw-trend', 'unavailable', 'metric-value warn');
    return;
  }

  const now = performance.now();
  const rawYawDeg = angles[2];
  const deltaYawDeg = lastImuYawRaw === null ? 0
    : ((rawYawDeg - lastImuYawRaw + 540) % 360) - 180;
  const unwrappedYawDeg = lastImuYawUnwrapped === null ? rawYawDeg
    : lastImuYawUnwrapped + deltaYawDeg;
  lastImuYawRaw = rawYawDeg;
  lastImuYawUnwrapped = unwrappedYawDeg;
  yawChangeSamples.push({ timeMs: now, yawDeg: unwrappedYawDeg });
  while (yawChangeSamples.length > 2
      && now - yawChangeSamples[0].timeMs > YAW_CHANGE_WINDOW_MS) {
    yawChangeSamples.shift();
  }

  const first = yawChangeSamples[0];
  const spanMs = now - first.timeMs;
  if (yawChangeSamples.length < 2 || spanMs < YAW_CHANGE_MIN_SPAN_MS) {
    setText('st-yaw-trend', 'collecting', 'metric-value warn');
    return;
  }

  const rateDegPerMinute = (unwrappedYawDeg - first.yawDeg) / (spanMs / 60_000);
  const rateText = `${rateDegPerMinute >= 0 ? '+' : ''}${rateDegPerMinute.toFixed(1)}°/min`;
  setText('st-yaw-trend', rateText);
}

/**
 * Set the connection indicator.
 *
 * @param {'ok'|'demo'|'bad'|'partial'} kind Indicator state.
 * @param {string} message Short status text.
 */
function setLink(kind, message) {
  const dot = document.getElementById('dot');
  const label = document.getElementById('st-conn');
  if (dot) dot.className = kind === 'ok' || kind === 'demo' || kind === 'partial' ? kind : '';
  if (label) label.textContent = message;
}

/**
 * Distinguish API availability from fresh tracking inputs and report stale feeds.
 *
 * @param {object} state Snapshot returned by the compute unit's `/state` route;
 *     freshness flags describe current IMU and ToF samples, and sample age is seconds.
 * @returns {void} Update the page's tracking-state badge without treating stale data as live.
 */
function setSensorLink(state) {
  if (state.mock === true) {
    setLink('demo', 'synthetic sensors');
    return;
  }
  const cameraState = state.camera ?? {};
  const hasCameraFix = cameraState.position_fresh === true
    && Array.isArray(cameraState.position_head_m)
    && Array.isArray(cameraState.rotation_head)
    && cameraState.rotation_head.length === 9;
  const cameraHasFrames = Number.isFinite(cameraState.frame_count)
    && cameraState.frame_count > 0;
  const cameraStreaming = cameraHasFrames && cameraFramesFresh !== false;
  const cameraHasDetections = Number.isFinite(cameraState.detection_count)
    && cameraState.detection_count > 0;
  const hasHead = state.head?.fresh === true
    && Array.isArray(state.head.euler_deg);
  const pointerState = state.pointer ?? null;
  const pointerPresent = pointerState !== null && typeof pointerState === 'object';
  const pointerFreshnessKnown = typeof pointerState?.imu_fresh === 'boolean'
    && typeof pointerState?.tof_fresh === 'boolean';
  const pointerAgeFresh = !Number.isFinite(pointerState?.sample_age_s)
    || pointerState.sample_age_s <= 0.25;
  const hasPointer = pointerPresent && pointerState.imu_fresh === true
    && pointerState.tof_fresh === true && pointerAgeFresh;
  const pointerStatus = !pointerPresent ? ''
    : hasPointer ? ''
      : pointerFreshnessKnown ? ' · pointer stale' : ' · pointer freshness unknown';
  if (hasCameraFix && hasPointer) {
    setLink(state.aligned === false ? 'partial' : 'ok',
      state.aligned === false ? 'align' : 'vision locked');
  } else if (hasCameraFix) {
    setLink('partial', 'vision only' + pointerStatus);
  } else if (hasHead && hasPointer) {
    setLink('partial', state.aligned === false ? 'align · IMU fallback' : 'IMU fallback');
  } else if (hasPointer) {
    setLink('partial', cameraHasFrames && !cameraStreaming
      ? 'pointer only · camera stalled' : 'pointer only');
  } else if (hasHead) {
    setLink('partial', 'head only' + pointerStatus);
  } else if (cameraStreaming) {
    setLink('partial', (cameraHasDetections ? 'vision stale' : 'searching tags')
      + pointerStatus);
  } else if (cameraHasFrames) {
    setLink('partial', `camera stalled${pointerStatus}`);
  } else if (pointerPresent) {
    setLink('partial', pointerFreshnessKnown ? 'pointer stale' : 'pointer freshness unknown');
  } else {
    setLink('partial', 'waiting for sensors');
  }
}

/** Track camera-frame progress and estimate its rate over a rolling time window.
 *
 * @param {object} camera Latest camera fields from the Pi snapshot; frame_count
 *     is a cumulative integer that advances once per captured frame.
 * @returns {void} Update the cached frame rate and freshness state.
 * @remarks Rate is averaged over up to five seconds to reduce burst jitter; a
 *     one-second stall threshold is diagnostic, not an accuracy measure.
 */
function trackCameraFrames(camera) {
  const frameCount = Number.isFinite(camera.frame_count) ? camera.frame_count : null;
  const now = performance.now();
  if (frameCount === null) {
    lastCameraFrameCount = null;
    lastCameraFrameAt = null;
    estimatedCameraFps = null;
    cameraFramesFresh = null;
    cameraFrameSamples = [];
    return;
  }

  if (lastCameraFrameCount === null) {
    lastCameraFrameCount = frameCount;
    lastCameraFrameAt = now;
    cameraFrameSamples = [{ atMs: now, count: frameCount }];
  } else if (frameCount > lastCameraFrameCount) {
    lastCameraFrameAt = now;
    lastCameraFrameCount = frameCount;
    cameraFrameSamples.push({ atMs: now, count: frameCount });
    const cutoffMs = now - CAMERA_RATE_WINDOW_MS;
    while (cameraFrameSamples.length > 2 && cameraFrameSamples[1].atMs < cutoffMs) {
      cameraFrameSamples.shift();
    }
    const firstSample = cameraFrameSamples[0];
    const elapsedMs = now - firstSample.atMs;
    estimatedCameraFps = elapsedMs >= CAMERA_RATE_WINDOW_MS * 0.8
      ? (frameCount - firstSample.count) * 1000 / elapsedMs : null;
  } else if (frameCount < lastCameraFrameCount) {
    lastCameraFrameCount = frameCount;
    lastCameraFrameAt = now;
    estimatedCameraFps = null;
    cameraFrameSamples = [{ atMs: now, count: frameCount }];
  }

  cameraFramesFresh = lastCameraFrameAt !== null
    && now - lastCameraFrameAt <= CAMERA_FRAME_STALE_MS;
  if (!cameraFramesFresh) estimatedCameraFps = null;
}

/**
 * Choose the snapshot origin for this page load.
 *
 * A page served by the compute unit talks to its own origin. A page opened from
 * a laptop static server is told apart by probing `/health`: the compute unit
 * and mock server answer it, a plain file server does not. An explicit
 * `?server=` wins, then a same-origin API, then the last saved server.
 *
 * @returns {Promise<string>} Base URL without a trailing slash.
 */
async function resolveServer() {
  const fromQuery = params.get('server');
  if (fromQuery) return fromQuery.replace(/\/$/, '');
  if (location.protocol === 'http:' || location.protocol === 'https:') {
    try {
      const probe = await fetch(`${location.origin}/health`, { cache: 'no-store' });
      if (probe.ok) return location.origin;
    } catch {
      // Not the compute unit or mock server; try the saved address next.
    }
  }
  const stored = localStorage.getItem(STORAGE_KEY);
  if (stored) return stored.replace(/\/$/, '');
  return DEFAULT_SERVER;
}

/**
 * Poll `/state` and redraw, one request at a time.
 *
 * A failed request keeps the last drawing and turns the indicator red; the next
 * success restores it, so a compute-unit restart needs no page reload.
 */
async function poll() {
  if (viewPaused || demoActive || pollInFlight) return;
  pollInFlight = true;
  const requestedAt = performance.now();
  pollStartSamples.push(requestedAt);
  pollStartSamples = pollStartSamples.filter(
    (startedAt) => requestedAt - startedAt <= POLL_RATE_WINDOW_MS,
  );
  const pollSpanMs = pollStartSamples.at(-1) - pollStartSamples[0];
  const pollHz = pollStartSamples.length > 1 ? (pollStartSamples.length - 1) * 1000 / pollSpanMs : null;
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), FETCH_TIMEOUT_MS);
  try {
    const base = serverUrl || location.origin;
    const response = await fetch(`${base}/state`, {
      cache: 'no-store',
      signal: controller.signal,
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const state = await response.json();
    const fetchMs = performance.now() - requestedAt;
    const rateLabel = pollHz === null ? 'sampling' : `${pollHz.toFixed(1)} Hz`;
    setText('st-fetch', `${fetchMs.toFixed(1)} ms · ${rateLabel}`);
    trackCameraFrames(state.camera ?? {});
    setSensorLink(state);
    setCameraFeed(base, state.camera ?? {});
    applyState(state);
    updateHud(state);
  } catch (error) {
    setLink('bad', error?.name === 'AbortError' ? 'timeout' : 'offline');
  } finally {
    clearTimeout(timeoutId);
    pollInFlight = false;
    const elapsedMs = performance.now() - requestedAt;
    const nextDelayMs = Math.max(0, POLL_INTERVAL_MS - elapsedMs);
    if (!viewPaused && !demoActive) setTimeout(poll, nextDelayMs);
  }
}

/**
 * Convert a rotation matrix to the row-major array the state schema uses.
 *
 * @param {THREE.Matrix4} matrix Rotation matrix.
 * @returns {number[]} Nine numbers in row-major order.
 */
function rowMajor(matrix) {
  const e = matrix.elements;
  return [e[0], e[4], e[8], e[1], e[5], e[9], e[2], e[6], e[10]];
}

/**
 * Build one synthetic snapshot for `?demo`.
 *
 * The geometry mirrors `audition.placement.place` with a 0.4 m default
 * baseline, so the page can be checked without hardware or a server.
 *
 * @param {number} t Seconds since the demo started.
 * @returns {object} A snapshot with the same shape as `/state`.
 */
function demoState(t) {
  const yaw = THREE.MathUtils.degToRad(55 * Math.sin(t * 0.6));
  const pitch = THREE.MathUtils.degToRad(18 * Math.sin(t * 0.37));
  const beam = new THREE.Vector3(
    Math.cos(pitch) * Math.cos(yaw),
    Math.cos(pitch) * Math.sin(yaw),
    Math.sin(pitch),
  ).normalize();
  const attitude = new THREE.Quaternion().setFromUnitVectors(X_AXIS, beam);
  const matrix = new THREE.Matrix4().makeRotationFromQuaternion(attitude);

  const range = 1.35 + 0.55 * Math.sin(t * 0.9);
  const origin = beam.clone().multiplyScalar(0.4);
  const hitPoint = origin.clone().addScaledVector(beam, range);
  const distance = hitPoint.length();
  const gain = Math.max(0, Math.min(1, 1 - distance / 4));
  const azimuth = THREE.MathUtils.radToDeg(Math.atan2(hitPoint.y, hitPoint.x));
  const elevation = THREE.MathUtils.radToDeg(Math.asin(hitPoint.z / distance));
  const pan = Math.max(-1, Math.min(1, -azimuth / 90));

  // The drawn pointer wanders around the head on its own inertial path,
  // as a real hand-held pointer would.
  const trackedOrigin = [
    0.34 + 0.14 * Math.sin(t * 0.8),
    0.22 * Math.sin(t * 0.23 + 1.1),
    0.12 + 0.11 * Math.sin(t * 0.5),
  ];

  return {
    t,
    aligned: true,
    muted: false,
    mock: true,
    config: { max_range_m: 4, full_scale_deg: 90 },
    head: { euler_deg: [0, 0, 0], fresh: true },
    pointer: {
      matrix_head: rowMajor(matrix),
      matrix_pointer_inertial: null,
      range_m: range,
      button: 0,
    },
    placement: {
      origin_m: origin.toArray(),
      beam: beam.toArray(),
      hit_m: hitPoint.toArray(),
      range_m: range,
      distance_m: distance,
      azimuth_deg: azimuth,
      elevation_deg: elevation,
      gain,
      pan,
      baseline: 'default',
      usable: true,
    },
    tracked: {
      origin_m: trackedOrigin,
      speed_mps: 0.3,
      holding: false,
      pointer_stale: false,
      clamped: false,
    },
    audio: { gain, pan },
    camera: { distance_m: null, age_s: null, position_head_m: null, position_fresh: false, marker_ids: [] },
  };
}

/**
 * Animate synthetic snapshots at display rate so the scene can be inspected
 * without hardware.
 */
function startDemo() {
  demoActive = true;
  const started = performance.now();
  setButtonLabel('toggle-demo', 'Stop demo');
  setLink('demo', 'demo');
  /** Render the next synthetic frame until demo mode is stopped. */
  const tick = () => {
    if (!demoActive) return;
    if (viewPaused) {
      requestAnimationFrame(tick);
      return;
    }
    const state = demoState((performance.now() - started) / 1000);
    applyState(state);
    updateHud(state);
    requestAnimationFrame(tick);
  };
  tick();
}

/**
 * Stop synthetic playback and resume live snapshots from the selected server.
 */
function stopDemo() {
  demoActive = false;
  setButtonLabel('toggle-demo', 'Start demo');
  setLink('bad', 'connecting');
  if (!viewPaused) poll();
}

/**
 * Apply the address entered in the connection field and restart live polling.
 *
 * @param {SubmitEvent} event Form submission event.
 */
function connectServer(event) {
  event.preventDefault();
  const input = document.getElementById('url');
  const entered = input ? input.value.trim().replace(/\/+$/, '') : '';
  serverUrl = entered;
  if (entered) localStorage.setItem(STORAGE_KEY, entered);
  else localStorage.removeItem(STORAGE_KEY);
  if (demoActive) stopDemo();
  viewPaused = false;
  setViewControls();
  setLink('bad', 'connecting');
  poll();
}

/**
 * Pause or resume browser updates without stopping the Pi pipeline.
 */
function togglePause() {
  viewPaused = !viewPaused;
  setViewControls();
  if (viewPaused) setLink('demo', 'paused');
  else if (demoActive) setLink('demo', 'demo');
  else poll();
}

/**
 * Synchronize pause controls with the current browser update state.
 */
function setViewControls() {
  const resume = document.getElementById('resume-view');
  setButtonLabel('pause-view', viewPaused ? 'Resume updates' : 'Pause updates');
  if (resume) resume.hidden = !viewPaused;
}

/**
 * Update a control's visible caption and accessible name together.
 *
 * @param {string} id Button element identifier.
 * @param {string} label Visible and screen-reader label.
 */
function setButtonLabel(id, label) {
  const button = document.getElementById(id);
  if (!button) return;
  const caption = button.querySelector('.button-label');
  if (caption) caption.textContent = label;
  else button.textContent = label;
  button.setAttribute('aria-label', label);
  button.title = label;
}

/**
 * Expand or collapse the mobile sensor drawer and expose its state to assistive technology.
 *
 * @param {boolean} expanded Whether to show the sensor values.
 */
function setSheetExpanded(expanded) {
  const sheet = document.getElementById('sidebar');
  const toggle = document.getElementById('sheet-toggle');
  const content = document.getElementById('sidebar-content');
  if (!sheet || !toggle || !content) return;
  sheet.classList.toggle('expanded', expanded);
  toggle.setAttribute('aria-expanded', String(expanded));
  content.setAttribute('aria-hidden', String(!expanded));
  content.inert = !expanded;
}

/**
 * Toggle the mobile sensor drawer from its pull-up tab.
 */
function toggleSheet() {
  const toggle = document.getElementById('sheet-toggle');
  setSheetExpanded(toggle?.getAttribute('aria-expanded') !== 'true');
}

/**
 * Track a touch or pen drag that begins on the drawer handle.
 *
 * @param {PointerEvent} event Pointer event in viewport pixels.
 */
function onSheetPointerDown(event) {
  if (window.innerWidth > 760) return;
  sheetPointer.startY = event.clientY;
  sheetPointer.moved = false;
}

/**
 * Expand the drawer after an upward swipe or collapse it after a downward swipe.
 *
 * @param {PointerEvent} event Pointer event in viewport pixels.
 */
function onSheetPointerUp(event) {
  if (sheetPointer.startY === null) return;
  const deltaY = event.clientY - sheetPointer.startY;
  sheetPointer.startY = null;
  if (Math.abs(deltaY) < 18) return;
  sheetPointer.moved = true;
  sheetPointer.suppressClick = true;
  setSheetExpanded(deltaY < 0);
}

/**
 * Clear drag tracking if the browser cancels a pointer gesture.
 */
function onSheetPointerCancel() {
  sheetPointer.startY = null;
}

/**
 * Prevent a completed drawer swipe from also firing the handle's click toggle.
 *
 * @param {MouseEvent} event Click generated after pointer release.
 */
function onSheetClick(event) {
  if (!sheetPointer.suppressClick) {
    toggleSheet();
    return;
  }
  sheetPointer.suppressClick = false;
  event.preventDefault();
}

/**
 * Show or hide the address editor on narrow screens.
 */
function toggleAddressEditor() {
  const form = document.getElementById('connection-form');
  const toggle = document.getElementById('connection-toggle');
  if (!form || !toggle) return;
  const open = form.classList.toggle('address-open');
  toggle.setAttribute('aria-expanded', String(open));
}

/**
 * Toggle the clearly labeled synthetic scene.
 */
function toggleDemo() {
  if (demoActive) stopDemo();
  else startDemo();
}

/**
 * Reframe the 3D camera around the current head and hit.
 */
function onFitView() {
  fitView();
}

/**
 * Resume live polling from the compact toolbar.
 */
function onResumeView() {
  viewPaused = false;
  setViewControls();
  if (demoActive) setLink('demo', 'demo');
  else poll();
}

/**
 * Collapse the camera preview on Escape or fit the scene on F.
 *
 * @param {KeyboardEvent} event Keyboard event.
 */
function onKeydown(event) {
  if (event.key === 'Escape') {
    setCameraPreviewExpanded(false);
    return;
  }
  if (event.key !== 'f' && event.key !== 'F') return;
  if (event.target instanceof HTMLInputElement) return;
  fitView();
}

/**
 * Match the drawing buffer to the canvas size.
 */
function resize() {
  const width = canvas.clientWidth || window.innerWidth;
  const height = canvas.clientHeight || window.innerHeight;
  renderer.setSize(width, height, false);
  camera.aspect = width / Math.max(height, 1);
  camera.updateProjectionMatrix();
}

/**
 * Render one frame; the renderer schedules the next.
 */
function animate() {
  controls.update();
  renderer.render(scene, camera);
}

/** Report that the proxied camera preview could not load. */
function onCameraFeedError() {
  const label = document.getElementById('camera-feed-state');
  if (label) label.textContent = 'feed unavailable';
}

/**
 * Expand or collapse the camera card for close marker inspection.
 *
 * @param {boolean} expanded Whether the preview uses the larger viewport layout.
 * @returns {void} Update the camera card and its accessible toggle state.
 */
function setCameraPreviewExpanded(expanded) {
  const card = document.getElementById('camera-card');
  const button = document.getElementById('camera-expand');
  if (!card || !button) return;
  card.classList.toggle('expanded', expanded);
  const label = expanded ? 'Shrink camera preview' : 'Expand camera preview';
  button.setAttribute('aria-expanded', String(expanded));
  button.setAttribute('aria-label', label);
  button.title = label;
}

/**
 * Toggle the camera card between compact and enlarged views.
 *
 * @returns {void} Update the camera card and accessible toggle label.
 */
function toggleCameraPreview() {
  const card = document.getElementById('camera-card');
  if (card) setCameraPreviewExpanded(!card.classList.contains('expanded'));
}

serverUrl = await resolveServer();

const urlInput = document.getElementById('url');
if (urlInput) {
  urlInput.value = serverUrl;
}

document.getElementById('connection-form')?.addEventListener('submit', connectServer);
document.getElementById('pause-view')?.addEventListener('click', togglePause);
document.getElementById('resume-view')?.addEventListener('click', onResumeView);
document.getElementById('fit-view')?.addEventListener('click', onFitView);
document.getElementById('recenter-view')?.addEventListener('click', onFitView);
document.getElementById('toggle-demo')?.addEventListener('click', toggleDemo);
document.getElementById('sheet-toggle')?.addEventListener('click', onSheetClick);
document.getElementById('sheet-toggle')?.addEventListener('pointerdown', onSheetPointerDown);
document.getElementById('sheet-toggle')?.addEventListener('pointerup', onSheetPointerUp);
document.getElementById('sheet-toggle')?.addEventListener('pointercancel', onSheetPointerCancel);
document.getElementById('connection-toggle')?.addEventListener('click', toggleAddressEditor);
document.getElementById('camera-expand')?.addEventListener('click', toggleCameraPreview);
document.getElementById('camera-feed')?.addEventListener('error', onCameraFeedError);
window.addEventListener('keydown', onKeydown);
setInterval(sampleCameraBrightness, 500);

setSheetExpanded(window.matchMedia('(max-width: 760px)').matches ? false : true);
resize();
window.addEventListener('resize', resize);
applyState({});
renderer.setAnimationLoop(animate);

if (params.has('demo')) {
  startDemo();
} else {
  poll();
}
