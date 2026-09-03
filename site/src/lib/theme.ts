import { createSignal } from "solid-js";

export type Theme = "dark" | "light";

function initialTheme(): Theme {
  if (typeof document !== "undefined") {
    return document.documentElement.dataset.theme === "light" ? "light" : "dark";
  }
  return "dark";
}

const [theme, setTheme] = createSignal<Theme>(initialTheme());

export { theme, setTheme };

export function toggleTheme() {
  const next: Theme = theme() === "dark" ? "light" : "dark";
  if (typeof document !== "undefined") {
    document.documentElement.dataset.theme = next;
  }
  try {
    localStorage.setItem("auris-theme", next);
  } catch {
    /* storage unavailable */
  }
  setTheme(next);
}
