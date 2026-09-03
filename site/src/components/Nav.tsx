import { theme, toggleTheme } from "~/lib/theme";
import s from "./Nav.module.css";

function SunIcon() {
  return (
    <svg
      width="18"
      height="18"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <circle cx="12" cy="12" r="5" />
      <path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42" />
    </svg>
  );
}

function MoonIcon() {
  return (
    <svg
      width="18"
      height="18"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
      stroke-linejoin="round"
      aria-hidden="true"
    >
      <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" />
    </svg>
  );
}

function Wordmark() {
  return (
    <svg width="22" height="22" viewBox="0 0 24 24" aria-hidden="true">
      <circle cx="7" cy="12" r="2.2" fill="var(--accent)" />
      <g
        fill="none"
        stroke="var(--accent)"
        stroke-width="2"
        stroke-linecap="round"
      >
        <path d="M12 7.5 A6.4 6.4 0 0 1 12 16.5" />
        <path d="M15.5 4.5 A10.6 10.6 0 0 1 15.5 19.5" />
      </g>
    </svg>
  );
}

const links = [
  { href: "#how-it-works", label: "How it works" },
  { href: "#features", label: "Features" },
  { href: "#tech", label: "Under the hood" },
  { href: "#roadmap", label: "Roadmap" }
];

export default function Nav() {
  return (
    <header class={s.header}>
      <div class={`container ${s.inner}`}>
        <a href="/" class={s.brand}>
          <Wordmark />
          <span class={s.name}>Auris</span>
        </a>

        <nav class={s.nav} aria-label="Main">
          {links.map((l) => (
            <a href={l.href} class={s.link}>
              {l.label}
            </a>
          ))}
        </nav>

        <div class={s.actions}>
          <a
            href="https://github.com/pewpil/auris"
            target="_blank"
            rel="noreferrer"
            class={s.github}
          >
            GitHub
          </a>
          <button
            type="button"
            class={s.themeToggle}
            onClick={toggleTheme}
            aria-label={`Switch to ${theme() === "dark" ? "light" : "dark"} theme`}
          >
            {theme() === "dark" ? <SunIcon /> : <MoonIcon />}
          </button>
        </div>
      </div>
    </header>
  );
}
