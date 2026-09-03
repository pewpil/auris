import s from "./HowItWorks.module.css";

const steps = [
  {
    n: "1",
    title: "The camera array sees the room",
    body: "A set of RGB cameras covers the space and tracks everything that matters: every object, and the user's head — full 6DoF, position and rotation — all registered into one shared world frame."
  },
  {
    n: "2",
    title: "Python computes the scene",
    body: "All spatial math lives in one pipeline. For every object it computes the head-relative azimuth, elevation and distance — thirty to sixty times a second."
  },
  {
    n: "3",
    title: "The headset renders the soundscape",
    body: "Unity receives those coordinates and places each object's earcon with HRTF binaural audio. The result is a room you can listen to."
  }
];

export default function HowItWorks() {
  return (
    <section id="how-it-works" class="section">
      <div class="container">
        <div class="section-head">
          <p class="eyebrow">How it works</p>
          <h2 class="section-title">Three steps, photon to sound wave</h2>
          <p class="section-lede">
            The pipeline is deliberately simple: cameras feed one scene model,
            one transform converts it to head-relative audio coordinates, and a
            headset renders it.
          </p>
        </div>
        <ol class={s.steps}>
          {steps.map((step) => (
            <li class={s.step}>
              <span class={s.number}>{step.n}</span>
              <h3 class={s.stepTitle}>{step.title}</h3>
              <p class={s.stepBody}>{step.body}</p>
            </li>
          ))}
        </ol>
      </div>
    </section>
  );
}
