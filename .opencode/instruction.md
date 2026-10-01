# Auris — Standing Instructions & Standards

General rules that apply to every session on this project.

## Section references (repository documents)

- In repository documents (`README.md`, `.opencode/*`, `docs/*`), cross-references to other sections should be written as **clickable anchor links** using GitHub's native kebab-case heading slugs — e.g. `[§7.2](#72-prototype-hardware-purchase-approval)` — never as bare unlinked text. (GitHub slug rule: lowercase, spaces → hyphens, punctuation dropped; `&` leaves a double hyphen.) This repo is read on GitHub, so GitHub-style slugs are the standard.

## Line wrapping (repository documents)

- Do **not** hard-wrap prose lines in `.md` files — let the renderer wrap. Each paragraph, list item, and blockquote paragraph is **one physical line**; break lines only where structure requires it: headings, list markers, table rows, fences and display-math blocks, and block separators (blank lines). Continuation lines of a paragraph or list item are joined into the item's single line.

## Math notation (repository documents)

- In any repository `.md` file, mathematical equations and expressions are always written in Markdown/LaTeX math notation — `$$…$$` for display equations and `$…$` for inline expressions (GitHub renders these natively) — never as plain-text approximations or code spans. Inside GitHub tables, avoid raw pipes inside math: use `\lVert … \rVert`-style commands instead of `|…|`.

## Math notation (AI chat responses)

- In replies to the user, do **not** use `$…$` or `$$…$$` — LaTeX does not render in the chat interface. Write math expressions wrapped in backticks (inline code spans, e.g., `D = |r|`, `theta = yaw_P - yaw_H`) or fenced code blocks instead of bare plain text. The `$` notation rule above applies to repository files only.

## Plan changes — trace the impact, write only on instruction

- The development plan is a living document: the co-researcher asks about hardware and the plan in their own sessions and may propose changes; nothing in it is locked, and the plan documents deliberately never use the word "frozen" — do not reintroduce freeze/lock wording into them. **The baseline-commit evaluation protocol is retired (2026-10-01)**: no change is checked against a frozen baseline commit before writing.
- What the retirement does *not* retire: before writing a change into the plan (`README.md`, `docs/*`, `.opencode/*`) — a hardware component or material selection, a design decision, a schedule shift, anything — trace what the change breaks across the **entire thesis project**, not only the system, and state the impact precisely and honestly: what **breaks**, what merely **degrades**, and which named plan sections absorb it. Be causal, not alarmist — e.g., skipping the bench tests does not disable writing the Methodology chapter; it removes the real results that thesis §4.1 and the pre-oral defense are scheduled to carry, shrinking the defended scope per the schedule's contingency ladder. Use the plan's own numbers (dates, thresholds, counts) wherever they exist. Dependency chains to check: the phase gates and bench-test matrix (T0–T8) against the thesis writing stages; the ≤ 100 ms motion-to-sound budget and its link-hop stage; the self-containment rule (no external runtime sources); the mandatory pointer-tracking stack; the purchasing constraints (connector-ready for P2, buy-once); the evaluation design (third revision — actual VI participants, within-subject one-visit design, no fallback); and the defense-anchored schedule (pre-oral Fri Dec 11, 2026; final Tue Apr 20, 2027).
- Do not write a change into the plan unless the co-researcher explicitly instructs it, and do not advertise that they may order a commit.
- **Current hardware state (2026-10-01):** the board class is **decided — the wearable is a Raspberry Pi 5, 4 GB**; the component and material selection is **complete and itemized** ([`docs/hardware.md`](../docs/hardware.md) selections; [`docs/purchase-list.md`](../docs/purchase-list.md) cart); the vision tier's camera count (**1 or 2**) is adopted at the T7 placement gate; the open lane is the co-researcher's purchase-approval sign-off, then checkout ([README §3](../README.md#3-hardware), [§9](../README.md#9-open-items)).
