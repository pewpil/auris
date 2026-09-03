import s from "./Hero.module.css";

function Object({
  cx,
  cy,
  label,
  labelDy
}: {
  cx: number;
  cy: number;
  label: string;
  labelDy: number;
}) {
  return (
    <g class={s.object}>
      <circle class={s.ring} cx={cx} cy={cy} r="8" />
      <circle class={`${s.ring} ${s.ringDelayed}`} cx={cx} cy={cy} r="8" />
      <circle class={s.core} cx={cx} cy={cy} r="6.5" />
      <text class={s.objectLabel} x={cx} y={cy + labelDy} text-anchor="middle">
        {label}
      </text>
    </g>
  );
}

function RoomDiagram() {
  return (
    <svg
      class={s.diagram}
      viewBox="0 0 640 440"
      role="img"
      aria-label="Wireframe diagram of a room: a person at the center, tracked objects around them, each emitting sound anchored to its position"
    >
      <rect class={s.room} x="16" y="16" width="608" height="408" rx="18" />

      <line class={s.shelf} x1="56" y1="120" x2="256" y2="120" />
      <text class={s.shelfLabel} x="56" y="106">
        shelf
      </text>

      <path
        class={s.cone}
        d="M330 270 L215.2 235.1 A120 120 0 0 1 264.4 169.5 Z"
      />

      <line class={s.measure} x1="330" y1="270" x2="430" y2="330" />
      <text class={s.measureLabel} x="368" y="290">
        φ · θ · r
      </text>

      <Object cx={110} cy={106} label="mug" labelDy={-16} />
      <Object cx={210} cy={106} label="book" labelDy={-16} />
      <Object cx={500} cy={210} label="chair" labelDy={26} />
      <Object cx={430} cy={330} label="bottle" labelDy={26} />
      <Object cx={130} cy={340} label="backpack" labelDy={26} />

      <circle class={s.head} cx="330" cy="270" r="13" />
      <text class={s.headLabel} x="330" y="304" text-anchor="middle">
        you
      </text>
    </svg>
  );
}

export default function Hero() {
  return (
    <section class={s.hero}>
      <div class={`container ${s.grid}`}>
        <div class={s.copy}>
          <p class={s.eyebrow}>Assistive spatial audio · thesis project</p>
          <h1 class={s.title}>
            Hear where
            <br />
            things are.
          </h1>
          <p class={s.lede}>
            Auris turns an ordinary room into a 3D soundscape. A camera array
            tracks every object and your head; a headset makes each object
            continuously emit its own distinctive sound — anchored to where it
            actually sits. Turn around, walk across the room: the sounds stay
            glued to the world.
          </p>
          <div class={s.ctas}>
            <a href="#how-it-works" class={s.primary}>
              See how it works
            </a>
            <a
              href="https://github.com/pewpil/auris"
              target="_blank"
              rel="noreferrer"
              class={s.ghost}
            >
              Read the concept
            </a>
          </div>
          <dl class={s.stats}>
            <div class={s.stat}>
              <dt>&lt; 100 ms</dt>
              <dd>end-to-end latency</dd>
            </div>
            <div class={s.stat}>
              <dt>6DoF</dt>
              <dd>head + objects, one world frame</dd>
            </div>
            <div class={s.stat}>
              <dt>30–60 Hz</dt>
              <dd>soundscape update rate</dd>
            </div>
          </dl>
        </div>

        <figure class={s.figure}>
          <RoomDiagram />
          <figcaption class={s.legend}>
            <span>
              <i class={`${s.dot} ${s.dotObject}`} /> object · earcon
            </span>
            <span>
              <i class={`${s.dot} ${s.dotRing}`} /> its sound
            </span>
            <span>
              <i class={`${s.dot} ${s.dotHead}`} /> where you face
            </span>
          </figcaption>
        </figure>
      </div>
    </section>
  );
}
