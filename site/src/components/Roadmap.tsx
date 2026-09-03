import s from "./Roadmap.module.css";

const milestones = [
  {
    phase: "Phases 1–3",
    name: "Prototype",
    status: "In progress",
    active: true,
    body: "The entire stack on contributed hardware: phone cameras streaming over WiFi, ARUCO markers as object stand-ins and a head rig. Validates the two core risks end-to-end."
  },
  {
    phase: "Phases 4–5",
    name: "Production",
    status: "Next",
    active: false,
    body: "Every component purchased on purpose-built gear: a camera array over the room, YOLO on real objects, markerless head tracking with an IMU fallback. Same contracts, new hardware."
  },
  {
    phase: "Phase 6",
    name: "Evaluation",
    status: "Planned",
    active: false,
    body: "A user study with blindfolded participants on the production system: angular error, time-to-locate, path efficiency — with ANOVA and t-tests."
  }
];

export default function Roadmap() {
  return (
    <section id="roadmap" class="section">
      <div class="container">
        <div class="section-head">
          <p class="eyebrow">Roadmap</p>
          <h2 class="section-title">From a rig of phones to a finished system</h2>
          <p class="section-lede">
            Development runs in two hardware stages split by a financial gate —
            the software contracts carry across untouched.
          </p>
        </div>
        <ol class={s.timeline}>
          {milestones.map((m) => (
            <li class={s.milestone}>
              <span
                class={`${s.dot} ${m.active ? s.dotActive : ""}`}
                aria-hidden="true"
              />
              <div class={s.card}>
                <div class={s.meta}>
                  <span class={s.phase}>{m.phase}</span>
                  <span
                    class={`${s.pill} ${m.active ? s.pillActive : s.pillMuted}`}
                  >
                    {m.status}
                  </span>
                </div>
                <h3 class={s.name}>{m.name}</h3>
                <p class={s.body}>{m.body}</p>
              </div>
            </li>
          ))}
        </ol>
      </div>
    </section>
  );
}
