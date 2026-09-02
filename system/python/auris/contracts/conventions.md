# Auris — Coordinate & Unit Conventions

The math bible for every backend (Prototype and Production) and for both wire
consumers (Python encoder, Unity decoder). The golden tests in
`tests/test_transform.py` enforce these definitions.

## World frame (the ONE shared room frame)

- Right-handed, origin fixed to the room by multi-camera extrinsic calibration.
- **Origin:** room floor center (calibration's choice; must be documented per rig).
- **+z up**; +x and +y span the floor plane (per-rig choice fixed at calibration).

## Head frame

- Right-handed, rigid with the user's head.
- **+x forward** (out of the face), **+y left** (over the left ear), **+z up**
  (through the crown).

## Head pose (6DoF)

`HeadPose` carries:

- `position_world` — p ∈ R³, meters, in the world frame.
- `rotation_matrix` — R ∈ SO(3) mapping head-frame vectors into world frame:
  `v_world = R · v_head`. Columns of R are the head axes expressed in world
  coordinates.
- `quaternion` — (w, x, y, z), Hamilton convention, the same world←head
  rotation: `v_world = q ⊙ (0, v_head) ⊙ q⁻¹`. Must match `rotation_matrix`.

Because the user walks, head pose is always full 6DoF — never rotation alone.

## Head-relative object coordinates (what SceneState carries)

For an object at `p_obj` and head pose (p, R):

```
d = p_obj − p            (world frame)
v = Rᵀ d                 (head frame: forward = v.x, left = v.y, up = v.z)
azimuth     φ = atan2(−v.y, v.x)          ∈ (−180°, 180°]
elevation   θ = atan2(v.z, hypot(v.x, v.y)) ∈ [−90°, +90°]
distance    r = ‖d‖                        ≥ 0
```

- **Azimuth 0 = straight ahead; positive to the user's right.**
- **Elevation positive above the horizon.**
- Distance in meters.

## Units on the wire

- Angles: **degrees** (f32). Positions/distances: **meters** (f32).
- Timestamps: monotonic clock **seconds** (f64); one clock domain per host.
- All wire integers/floats are **little-endian**.
- Quaternion order on the wire: **(w, x, y, z)**; Unity consumers repack to
  `UnityEngine.Quaternion(x, y, z, w)`.

## Invalid-head semantics

When a SceneState has no valid head pose (`FLAG_HEAD_VALID` clear), the
per-object azimuth/elevation/distance fields are **undefined (zero-filled)**;
consumers must render nothing or hold their last-good state. World positions
remain valid.

## Frame semantics

Azimuth/elevation/distance are relative to the **current** head pose — Python
performs the full 6DoF transform; Unity never recomputes poses (dumb renderer).
