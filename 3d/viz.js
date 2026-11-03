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
 *                 by the compute unit, otherwise the bench address below.
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

/** How often the page asks the compute unit for a new snapshot, ms. */
const POLL_INTERVAL_MS = 50;

/** Snapshot origin used when this page is opened from a local file server. */
const DEFAULT_SERVER = 'http://192.168.77.2:8081';

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
 * The pointer cube sits at the inertially tracked position when the compute
 * unit publishes one (`tracked.origin_m`), falling back to the audio
 * placement's origin when it does not. The beam and the hit follow the drawn
 * origin, so the drawn geometry is always the beam leaving the drawn pointer.
 *
 * @param {object} state Snapshot from the compute unit's `/state`.
 */
function applyState(state) {
  lastState = state;
  const placement = state.placement ?? null;
  const pointerState = state.pointer ?? null;
  const trackedState = state.tracked ?? null;
  const origin = trackedState && trackedState.origin_m
    ? new THREE.Vector3().fromArray(trackedState.origin_m)
    : (placement && placement.origin_m
      ? new THREE.Vector3().fromArray(placement.origin_m)
      : new THREE.Vector3());
  const usable = Boolean(
    placement && placement.usable && placement.beam
    && Number.isFinite(placement.range_m),
  );

  const pointerVisible = Boolean(pointerState && pointerState.matrix_head);
  pointerGroup.visible = pointerVisible;
  pointerGroup.position.copy(origin);
  lastOrigin.copy(origin);
  pointerMaterial.opacity = usable ? 1 : 0.35;
  pointerEdgeMaterial.opacity = usable ? 0.9 : 0.3;
  if (pointerVisible) {
    setRotationMatrix(rotationMatrix, pointerState.matrix_head);
    pointerGroup.quaternion.setFromRotationMatrix(rotationMatrix);
  }

  const baselineLength = origin.length();
  baselineLine.visible = pointerVisible && baselineLength > 1e-3;
  if (baselineLine.visible) {
    setLine(baselineLine, new THREE.Vector3(), origin);
  }

  if (!usable) {
    beamMesh.visible = false;
    hitMesh.visible = false;
    sightLine.visible = false;
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
 * Write one HUD value.
 *
 * @param {string} id Element id.
 * @param {string} text Text to show.
 * @param {string} [className] Optional colour class.
 */
function setText(id, text, className = 'v') {
  const element = document.getElementById(id);
  if (!element) return;
  element.textContent = text;
  element.className = className;
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
  const euler = head.euler_deg;

  setText('st-head', euler
    ? `r ${signed(euler[0])}  p ${signed(euler[1])}  y ${signed(euler[2])}`
    : '—');
  setText('st-range', pointer && Number.isFinite(pointer.range_m)
    ? `${pointer.range_m.toFixed(2)} m` : '—');
  setText('st-beam', placement
    ? `az ${signed(placement.azimuth_deg)}  el ${signed(placement.elevation_deg)}`
    : '—');
  setText('st-dist', placement && Number.isFinite(placement.distance_m)
    ? `${placement.distance_m.toFixed(2)} m` : '—');
  setText('st-gain', placement ? fmt(placement.gain, 3) : '—');
  setText('st-pan', placement ? signed(placement.pan, 2) : '—');

  const baseline = placement ? placement.baseline : (state.aligned ? 'waiting' : 'unaligned');
  const baselineClass = baseline === 'camera' ? 'v good'
    : baseline === 'unaligned' || baseline === 'waiting' ? 'v warn' : 'v';
  setText('st-base', baseline, baselineClass);

  setText('st-cam', Number.isFinite(cameraFix.distance_m)
    ? `${cameraFix.distance_m.toFixed(2)} m · ${fmt(cameraFix.age_s, 2)} s`
    : 'no marker');
  setText('st-btn', pointer ? (pointer.button ? 'down' : 'up') : '—');
  setText('st-audio', state.muted ? 'muted' : 'on',
    state.muted ? 'v warn' : 'v good');

  const tracked = state.tracked ?? null;
  if (tracked && tracked.origin_m) {
    const radius = Math.hypot(tracked.origin_m[0], tracked.origin_m[1],
      tracked.origin_m[2]);
    const flags = tracked.pointer_stale ? ' · stale'
      : tracked.holding ? ' · hold'
      : tracked.clamped ? ' · clamp' : '';
    setText('st-track', `${radius.toFixed(2)} m${flags}`,
      tracked.pointer_stale || tracked.clamped ? 'v warn' : 'v');
  } else {
    setText('st-track', '—');
  }

  if (state.aligned === false) {
    setText('st-head', 'align first', 'v warn');
  }
}

/**
 * Set the connection indicator.
 *
 * @param {'ok'|'demo'|'bad'} kind Indicator state.
 * @param {string} message Short status text.
 */
function setLink(kind, message) {
  const dot = document.getElementById('dot');
  const label = document.getElementById('st-conn');
  if (dot) dot.className = kind === 'ok' ? 'ok' : kind === 'demo' ? 'demo' : '';
  if (label) label.textContent = message;
}

/**
 * Choose the snapshot origin for this page load.
 *
 * A page served by the compute unit talks to its own origin. A page opened from
 * a laptop static server is told apart by probing `/health`: the compute unit
 * answers it, a plain file server does not. An explicit `?server=` or the last
 * URL typed always wins.
 *
 * @returns {Promise<string>} Base URL without a trailing slash.
 */
async function resolveServer() {
  const fromQuery = params.get('server');
  if (fromQuery) return fromQuery.replace(/\/$/, '');
  const stored = localStorage.getItem(STORAGE_KEY);
  if (stored) return stored.replace(/\/$/, '');
  if (location.protocol === 'http:' || location.protocol === 'https:') {
    try {
      const probe = await fetch(`${location.origin}/health`, { cache: 'no-store' });
      if (probe.ok) return location.origin;
    } catch {
      // Not the compute unit; fall through to the bench address.
    }
  }
  return DEFAULT_SERVER;
}

/**
 * Poll `/state` and redraw, one request at a time.
 *
 * A failed request keeps the last drawing and turns the indicator red; the next
 * success restores it, so a compute-unit restart needs no page reload.
 */
async function poll() {
  try {
    const response = await fetch(`${serverUrl}/state`, { cache: 'no-store' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const state = await response.json();
    setLink('ok', 'live');
    applyState(state);
    updateHud(state);
  } catch {
    setLink('bad', 'no data');
  } finally {
    setTimeout(poll, POLL_INTERVAL_MS);
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
    config: { max_range_m: 4, full_scale_deg: 90 },
    head: { euler_deg: [0, 0, 0], fresh: true },
    pointer: { matrix_head: rowMajor(matrix), range_m: range, button: 0 },
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
    camera: { distance_m: null, age_s: null },
  };
}

/**
 * Animate synthetic snapshots at display rate so the scene can be inspected
 * without hardware.
 */
function startDemo() {
  const started = performance.now();
  setLink('demo', 'demo');
  const tick = () => {
    const state = demoState((performance.now() - started) / 1000);
    applyState(state);
    updateHud(state);
    requestAnimationFrame(tick);
  };
  tick();
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

serverUrl = await resolveServer();

const urlInput = document.getElementById('url');
if (urlInput) {
  urlInput.value = serverUrl;
  urlInput.addEventListener('change', () => {
    serverUrl = (urlInput.value.trim() || DEFAULT_SERVER).replace(/\/$/, '');
    urlInput.value = serverUrl;
    localStorage.setItem(STORAGE_KEY, serverUrl);
    setLink('bad', 'connecting');
  });
}

window.addEventListener('keydown', (event) => {
  if (event.key !== 'f' && event.key !== 'F') return;
  if (event.target instanceof HTMLInputElement) return;
  fitView();
});

resize();
window.addEventListener('resize', resize);
applyState({});
renderer.setAnimationLoop(animate);

if (params.has('demo')) {
  startDemo();
} else {
  poll();
}
