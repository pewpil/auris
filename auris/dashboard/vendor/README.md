# Vendored three.js

Pinned copies of the browser libraries the debug view needs, so the page works on the isolated bench network with no internet access. The versions match: three.js and its examples are always taken from the same release.

| File | Version | Source |
|---|---|---|
| `three.module.min.js` | r160 (`0.160.0`) | `build/three.module.min.js` from the `three` npm package |
| `OrbitControls.js` | r160 (`0.160.0`) | `examples/jsm/controls/OrbitControls.js` from the same package |

Both are MIT-licensed, © 2010-2023 three.js authors. `OrbitControls.js` imports from the bare specifier `three`, which `index.html` resolves to `three.module.min.js` through an import map.

To upgrade, replace both files from the same release and update this table; the page uses no other addons.
