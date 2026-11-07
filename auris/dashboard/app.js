import * as THREE from 'three';
import { OrbitControls } from './vendor/OrbitControls.js';

let sceneView = null;
let polling = false;
let timer = null;
let lastState = null;
let lastFrame = null;
let lastFrameAt = 0;
let cameraFailed = false;
const byId = document.getElementById.bind(document);

/** Write a text value without interpreting telemetry as HTML. @param {string} id Element ID. @param {string} value Display text. @returns {void} */
function text(id, value) { byId(id).textContent = value; }
/** Validate a finite three-component vector. @param {unknown} v Candidate vector in the caller's units/frame. @returns {boolean} Whether all three components are finite. */
function vector(v) { return Array.isArray(v) && v.length === 3 && v.every(Number.isFinite); }
/** Format XYZ components in their supplied units/frame. @param {number[]} v XYZ vector. @param {string} unit Display unit. @returns {string} Formatted vector or unavailable marker. */
function xyz(v, unit = '') { return vector(v) ? `${v.map(fixed).join(' / ')} ${unit}`.trim() : 'Not reported'; }
/** Format a finite scalar to two decimals. @param {number} n Scalar in caller units. @returns {string} Decimal text. */
function fixed(n) { return n.toFixed(2); }
/** Set a status label and color. @param {string} id Element ID. @param {string} label Status text. @param {string} tone Semantic status color. @returns {void} */
function status(id, label, tone = 'off') { text(id, label); byId(id).dataset.tone = tone; }
/** Extract ZYX Euler angles from a row-major body-to-reference matrix. @param {number[]} m Unitless 3×3 rotation. @returns {number[]|null} Roll/pitch/yaw in degrees, or null. */
function euler(m) {
  if (!Array.isArray(m) || m.length !== 9 || !m.every(Number.isFinite)) return null;
  return [Math.atan2(m[7], m[8]), Math.asin(Math.max(-1, Math.min(1, -m[6]))), Math.atan2(m[3], m[0])].map(degrees);
}
/** Convert radians to degrees. @param {number} n Angle in radians. @returns {number} Angle in degrees. */
function degrees(n) { return n * 180 / Math.PI; }
/** Convert standard gravity units to m/s². @param {number} n Acceleration in g. @returns {number} Acceleration in m/s². */
function acceleration(n) { return n * 9.80665; }
/** Render one IMU in its published reference frame. @param {string} key Card prefix. @param {object} sensor Sensor telemetry. @param {boolean} online API availability. @returns {void} */
function renderImu(key, sensor, online) {
  const angles = key === 'head' ? sensor.euler_deg : euler(sensor.matrix_pointer_inertial ?? sensor.matrix_head);
  const valid = vector(angles);
  const age = key === 'head' ? sensor.age_s : sensor.sample_age_s;
  const fresh = online && (key === 'head' ? sensor.fresh === true : sensor.imu_fresh === true) && (!Number.isFinite(age) || age <= .25);
  status(`${key}-status`, fresh ? 'Live' : valid ? 'Stale' : 'No data', fresh ? 'good' : valid ? 'warn' : 'off');
  /** Render one Euler component in degrees. @param {string} axis Euler axis name. @param {number} index Component index. @returns {void} */
  function renderAngle(axis, index) {
    text(`${key}-${axis}`, valid ? `${angles[index].toFixed(1)}°` : '—');
  }
  ['roll', 'pitch', 'yaw'].forEach(renderAngle);
  byId(`${key}-arrow`).setAttribute('transform', valid ? `rotate(${-angles[2]} 90 65)` : '');
  byId(`${key}-arrow`).style.opacity = fresh ? '1' : '.25';
  const accel = vector(sensor.accel_mps2) ? sensor.accel_mps2 : vector(sensor.accel_g) ? sensor.accel_g.map(acceleration) : null;
  text(`${key}-accel`, xyz(accel));
  text(`${key}-gyro`, xyz(sensor.gyro_dps));
  const frame = key === 'pointer' && sensor.matrix_pointer_inertial ? 'Local IMU frame · yaw unregistered' : 'Published sensor frame';
  text(`${key}-detail`, `${frame}${Number.isFinite(age) ? ` · ${(age * 1000).toFixed(0)} ms old` : ''}${!online ? ' · connection lost' : ''}. Arrow shows yaw; pitch and roll appear above.`);
}
/** Draw tag visibility only from fresh camera reports. @param {object} camera Camera telemetry. @param {boolean} fresh Camera frame freshness. @returns {void} */
function renderTags(camera, fresh) {
  const configured = Array.isArray(camera.target_marker_ids) ? camera.target_marker_ids.filter(Number.isInteger) : [];
  const seen = Array.isArray(camera.marker_ids) ? camera.marker_ids.filter(Number.isInteger) : [];
  const ids = [...new Set([...configured, ...seen])];
  byId('tags').replaceChildren();
  for (const id of ids) {
    const row = document.createElement('div'); row.className = 'tag-row';
    const visible = fresh && camera.position_fresh === true && seen.includes(id);
    row.dataset.visible = String(visible);
    const label = document.createElement('strong'); label.textContent = `ID ${id}`;
    const value = document.createElement('span'); value.textContent = !fresh ? 'Unknown' : visible ? 'Visible' : 'Not visible';
    row.append(label, value); byId('tags').append(row);
  }
  if (!ids.length) text('tags', 'No configured tag IDs reported.');
  status('tag-status', fresh ? 'Live camera' : 'Unknown', fresh ? 'good' : 'off');
  text('tag-detail', 'Configured IDs from the detector; stale frames never count as visible.');
}
/** Create an orbitable scene in metres with +X forward, +Y left and +Z up. @returns {object} Scene objects; throws if WebGL initialization fails. */
function createScene() {
  const canvas = byId('scene');
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
  const scene = new THREE.Scene(); scene.background = new THREE.Color('#ebebeb');
  const camera = new THREE.PerspectiveCamera(45, 1, .01, 100); camera.up.set(0, 0, 1);
  const controls = new OrbitControls(camera, canvas); controls.enableDamping = true;
  const grid = new THREE.GridHelper(6, 12, '#ccd3df', '#e5e9ef'); grid.rotation.x = Math.PI / 2; grid.position.z = -.15; scene.add(grid);
  const head = new THREE.Mesh(new THREE.SphereGeometry(.11, 24, 16), new THREE.MeshBasicMaterial({ color: '#7d98c0' })); scene.add(head);
  scene.add(new THREE.ArrowHelper(new THREE.Vector3(1, 0, 0), new THREE.Vector3(), .28, '#486c9a', .07, .04));
  const probe = new THREE.Mesh(new THREE.BoxGeometry(.14, .06, .06), new THREE.MeshBasicMaterial({ color: '#6d59db', wireframe: true })); scene.add(probe);
  const hit = new THREE.Mesh(new THREE.SphereGeometry(.035, 16, 12), new THREE.MeshBasicMaterial({ color: '#e7b41b' })); scene.add(hit);
  const baseline = makeLine('#7050ff', scene), beam = makeLine('#ff0024', scene), sight = makeLine('#f000f5', scene);
  const view = { renderer, scene, camera, controls, probe, hit, baseline, beam, sight };
  resetView(view); hideGeometry(view);
  /** Resize the renderer to its CSS container in pixels. @returns {void} */
  function resizeScene() {
    const { width, height } = canvas.parentElement.getBoundingClientRect();
    renderer.setSize(width, height, false); camera.aspect = width / Math.max(1, height); camera.updateProjectionMatrix();
  }
  const observer = new ResizeObserver(resizeScene); observer.observe(canvas.parentElement);
  /** Render one display frame and update orbit damping. @returns {void} */
  function drawScene() {
    controls.update(); renderer.render(scene, camera);
  }
  renderer.setAnimationLoop(drawScene);
  return view;
}
/** Create a colored geometry line. @param {string} color CSS color. @param {THREE.Scene} scene Owning scene. @returns {THREE.Line} Two-point line in metres. */
function makeLine(color, scene) {
  const line = new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(), new THREE.Vector3()]), new THREE.LineBasicMaterial({ color })); scene.add(line); return line;
}
/** Set a line's endpoints in the head frame. @param {THREE.Line} line Target line. @param {THREE.Vector3} a Start in metres. @param {THREE.Vector3} b End in metres. @returns {void} */
function lineBetween(line, a, b) { line.geometry.setFromPoints([a, b]); line.visible = true; }
/** Restore the default orbit camera. @param {object} view Scene bundle. @returns {void} */
function resetView(view) { view.camera.position.set(-1.4, -2.5, 1.8); view.controls.target.set(.6, 0, 0); view.controls.update(); }
/** Hide unavailable probe and hit geometry. @param {object} view Scene bundle. @returns {void} */
function hideGeometry(view) { for (const key of ['probe', 'hit', 'baseline', 'beam', 'sight']) view[key].visible = false; }
/** Render validated head-relative placement, never inventing an origin. @param {object} state API snapshot. @param {boolean} online API availability. @returns {void} */
function renderGeometry(state, online) {
  const p = state.placement ?? {}, pointer = state.pointer ?? {};
  const rangeFresh = online && pointer.tof_fresh === true && Number.isFinite(pointer.range_m) && pointer.range_m >= 0 && (!Number.isFinite(pointer.range_age_ms) || pointer.range_age_ms <= 250);
  text('ray-distance', rangeFresh ? `${(pointer.range_m * 100).toFixed(1)} cm` : 'Unavailable / stale');
  text('probe-position', 'Unavailable'); text('hit-distance', 'Unavailable');
  if (sceneView) hideGeometry(sceneView);
  const valid = online && p.usable === true && vector(p.origin_m) && vector(p.beam) && Number.isFinite(p.range_m) && p.range_m >= 0;
  text('scene-message', !sceneView ? '3D unavailable: WebGL is not supported' : valid ? 'Head-relative placement · metres' : 'Waiting for usable head-relative placement');
  if (!valid) return;
  const origin = new THREE.Vector3().fromArray(p.origin_m), direction = new THREE.Vector3().fromArray(p.beam);
  if (direction.lengthSq() < 1e-12) return;
  direction.normalize(); const hit = origin.clone().addScaledVector(direction, p.range_m);
  text('ray-distance', `${(p.range_m * 100).toFixed(1)} cm`);
  text('probe-position', xyz(p.origin_m, 'm · X/Y/Z'));
  text('hit-distance', `${(hit.length() * 100).toFixed(1)} cm`);
  if (!sceneView) return;
  const v = sceneView; v.probe.visible = true; v.hit.visible = true; v.probe.position.copy(origin); v.hit.position.copy(hit);
  v.probe.quaternion.setFromUnitVectors(new THREE.Vector3(1, 0, 0), direction);
  lineBetween(v.baseline, new THREE.Vector3(), origin); lineBetween(v.beam, origin, hit); lineBetween(v.sight, new THREE.Vector3(), hit);
}
/** Render camera health using progression of the frame counter. @param {object} camera Camera telemetry. @param {boolean} online API availability. @returns {boolean} Whether camera frames are fresh. */
function renderCamera(camera, online) {
  const now = performance.now();
  if (Number.isFinite(camera.frame_count) && camera.frame_count > 0 && camera.frame_count !== lastFrame) { lastFrame = camera.frame_count; lastFrameAt = now; }
  const fresh = online && lastFrameAt > 0 && now - lastFrameAt < 1500;
  const image = byId('camera-feed');
  if (fresh && !image.getAttribute('src') && !cameraFailed) image.src = '/camera';
  image.hidden = !fresh || cameraFailed;
  byId('camera-message').hidden = fresh && !cameraFailed;
  text('camera-message', cameraFailed ? 'Camera stream unavailable · refresh to retry' : 'Waiting for fresh camera frames');
  status('camera-status', fresh && !cameraFailed ? 'Live' : 'Unavailable / stale', fresh && !cameraFailed ? 'good' : 'off');
  text('camera-detail', `Frames: ${camera.frame_count ?? '—'} · accepted poses: ${camera.detection_count ?? '—'} · read failures: ${camera.read_failures ?? '—'}`);
  return fresh;
}
/** Render a complete live or disconnected snapshot. @param {object} state Compute-unit state. @param {boolean} online API availability. @param {number} rtt Round trip milliseconds. @returns {void} */
function render(state, online, rtt = 0) {
  renderImu('head', state.head ?? {}, online); renderImu('pointer', state.pointer ?? {}, online);
  renderTags(state.camera ?? {}, renderCamera(state.camera ?? {}, online)); renderGeometry(state, online);
  status('connection-label', online ? state.mock ? 'Synthetic demo data' : 'Compute unit connected' : 'Compute unit offline');
  byId('connection').dataset.tone = online ? state.mock ? 'warn' : 'good' : 'bad';
  text('source', state.mock ? 'Synthetic data' : state.pointer?.source ?? state.source ?? 'Pi telemetry');
  if (online) { text('updated', new Date().toLocaleTimeString()); text('rtt', `${rtt.toFixed(1)} ms`); }
}
/** Poll without overlap and bound requests to 2.5 seconds. @returns {Promise<void>} Resolve after rendering; network errors become offline status. */
async function poll() {
  if (polling) return; polling = true;
  const started = performance.now();
  try {
    const response = await fetch('/state', { cache: 'no-store', signal: AbortSignal.timeout(2500) });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const snapshot = await response.json();
    if (!snapshot || typeof snapshot !== 'object' || Array.isArray(snapshot)) throw new Error('Invalid state snapshot');
    lastState = snapshot; render(lastState, true, performance.now() - started);
    text('footer-message', lastState.mock ? 'Synthetic source · demonstration only' : 'Live telemetry from the compute unit');
  } catch (error) { render(lastState ?? {}, false); text('footer-message', `No live snapshot: ${error.message}`); }
  finally { polling = false; timer = setTimeout(poll, 100); }
}
/** Retry camera loading and request fresh telemetry. @returns {void} */
function refresh() { clearTimeout(timer); cameraFailed = false; byId('camera-feed').removeAttribute('src'); poll(); }
/** Mark an unsuccessful camera stream unavailable. @returns {void} */
function cameraError() { cameraFailed = true; byId('camera-feed').hidden = true; byId('camera-message').hidden = false; text('camera-message', 'Camera stream unavailable · refresh to retry'); }
/** Reset the scene from the operator control. @returns {void} */
function reset() { if (sceneView) resetView(sceneView); }
try { sceneView = createScene(); } catch { text('scene-message', '3D unavailable: WebGL is not supported'); }
byId('refresh').addEventListener('click', refresh);
byId('reset-view').addEventListener('click', reset);
byId('camera-feed').addEventListener('error', cameraError);
poll();

