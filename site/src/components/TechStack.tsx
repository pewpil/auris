import s from "./TechStack.module.css";

const layers = [
  {
    role: "Perception",
    chips: ["Python", "OpenCV", "YOLO", "ARUCO", "Kalman / ByteTrack"]
  },
  {
    role: "Audio",
    chips: ["Unity", "HRTF", "3D Tune-In Toolkit", "Steam Audio"]
  },
  {
    role: "Transport",
    chips: ["UDP", "msgpack", "30–60 Hz", "localhost → LAN"]
  }
];

const packet = `SceneState  ·  30–60 Hz over UDP  ·  frozen in the prototype
{
  "objects": [
    { "id": "bottle", "azimuth":  -38.2, "elevation": 12.0, "distance": 2.41 },
    { "id": "mug",    "azimuth":   71.5, "elevation": -4.3, "distance": 1.08 },
    { "id": "book",   "azimuth": -102.0, "elevation":  2.1, "distance": 3.15 }
  ]
}`;

export default function TechStack() {
  return (
    <section id="tech" class="section">
      <div class="container">
        <div class="section-head">
          <p class="eyebrow">Under the hood</p>
          <h2 class="section-title">One contract between the brains and the ears</h2>
          <p class="section-lede">
            All spatial math lives in Python; Unity stays a dumb renderer. The
            two talk through a single frozen schema — so hardware can be
            swapped without touching either side.
          </p>
        </div>
        <div class={s.grid}>
          <div class={s.layers}>
            {layers.map((l) => (
              <div class={s.layer}>
                <span class={s.role}>{l.role}</span>
                <div class={s.chips}>
                  {l.chips.map((c) => (
                    <span class={s.chip}>{c}</span>
                  ))}
                </div>
              </div>
            ))}
            <p class={s.note}>
              Prototype runs phone cameras and ARUCO markers; production swaps
              in a camera array, YOLO on real objects and markerless head
              tracking. The packet below never changes.
            </p>
          </div>
          <pre class={s.packet}>
            <code>{packet}</code>
          </pre>
        </div>
      </div>
    </section>
  );
}
