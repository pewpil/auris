import s from "./Footer.module.css";

export default function Footer() {
  return (
    <footer class={s.footer}>
      <div class={`container ${s.inner}`}>
        <div class={s.brandCol}>
          <span class={s.name}>Auris</span>
          <p class={s.tagline}>
            An assistive-technology thesis project: computer vision and spatial
            audio that help visually impaired users find objects by sound
            alone.
          </p>
        </div>
        <nav class={s.links} aria-label="Footer">
          <a
            href="https://github.com/pewpil/auris"
            target="_blank"
            rel="noreferrer"
          >
            GitHub
          </a>
          <a href="#how-it-works">How it works</a>
          <a href="#roadmap">Roadmap</a>
        </nav>
      </div>
      <div class={`container ${s.legal}`}>
        <small>© 2026 The Auris project · site is a working wireframe</small>
      </div>
    </footer>
  );
}
