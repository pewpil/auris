import type { JSX } from "solid-js";
import s from "./Features.module.css";

function IconRotate() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="1.8"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <path d="M1 4v6h6" />
      <path d="M3.51 15a9 9 0 1 0 2.13-9.36L1 10" />
    </svg>
  );
}

function IconCrosshair() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="1.8"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <circle cx="12" cy="12" r="7" />
      <path d="M12 2v3M12 19v3M2 12h3M19 12h3" />
    </svg>
  );
}

function IconBeacon() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="1.8"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <circle cx="12" cy="14" r="2" fill="currentColor" stroke="none" />
      <path d="M8.5 10.5a5 5 0 0 1 7 0" />
      <path d="M5.8 7.8a9 9 0 0 1 12.4 0" />
    </svg>
  );
}

function IconBars() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="1.8"
      stroke-linecap="round"
      aria-hidden="true"
    >
      <path d="M5 10v4M12 5v14M19 8v8" />
    </svg>
  );
}

function IconClock() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="1.8"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <circle cx="12" cy="13" r="8" />
      <path d="M12 9v4l2.5 2.5M9 2h6" />
    </svg>
  );
}

function IconSwap() {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="1.8"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <path d="M17 1l4 4-4 4" />
      <path d="M3 11V9a4 4 0 0 1 4-4h14" />
      <path d="M7 23l-4-4 4-4" />
      <path d="M21 13v2a4 4 0 0 1-4 4H3" />
    </svg>
  );
}

const features: Array<{ icon: () => JSX.Element; title: string; body: string }> = [
  {
    icon: IconRotate,
    title: "World-anchored soundscapes",
    body: "Turn your head or walk the room and the sound field rotates and moves with you. Every sound stays fixed to the object that emits it."
  },
  {
    icon: IconCrosshair,
    title: "Full 6DoF head tracking",
    body: "Your head is tracked for position and rotation at 30–60 Hz, smoothed with a One-Euro filter so the audio never jitters or jumps."
  },
  {
    icon: IconBeacon,
    title: "Beacon mode",
    body: "Ask for an object — \u201Cfind the bottle\u201D — and it answers: a periodic ping you can home in on, like a lighthouse for your stuff."
  },
  {
    icon: IconBars,
    title: "Always-on ambient awareness",
    body: "Objects don't wait to be asked. Each one whispers its presence continuously — a bubbling bottle, a chiming mug, a rustling book."
  },
  {
    icon: IconClock,
    title: "Under 100 ms, end to end",
    body: "From photon to sound wave in under a tenth of a second — a hard engineering target that is verified on every hardware path, not assumed."
  },
  {
    icon: IconSwap,
    title: "Swap-ready hardware",
    body: "Cameras, trackers and audio devices sit behind stable contracts. The prototype upgrades to the production build without touching the pipeline."
  }
];

export default function Features() {
  return (
    <section id="features" class="section">
      <div class="container">
        <div class="section-head">
          <p class="eyebrow">Features</p>
          <h2 class="section-title">A room you can listen to</h2>
          <p class="section-lede">
            Auris combines continuous ambient sound with on-demand beacons, so
            a whole room stays legible by ear alone.
          </p>
        </div>
        <div class={s.grid}>
          {features.map((f) => (
            <article class={s.card}>
              <span class={s.icon}>
                <f.icon />
              </span>
              <h3 class={s.cardTitle}>{f.title}</h3>
              <p class={s.cardBody}>{f.body}</p>
            </article>
          ))}
        </div>
      </div>
    </section>
  );
}
