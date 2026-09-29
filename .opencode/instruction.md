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

## Plan freeze — evaluate every change against the baseline before writing

- The development plan is frozen in substance against the baseline commit `91148818d416f94080e0e6a0de6531ff67d941d1` ("link layer genericized"). The plan documents deliberately never use the word "frozen" — do not reintroduce freeze/lock wording into them. The hardware selection is **open again (cleared 2026-09-29, second clearing)**: the co-researcher sources components and materials **compatible with both esp32-s3 and raspi-5** before deciding the board class ([README §3](../README.md#3-hardware) constraint 4, [§9](../README.md#9-open-items)); the components-and-materials record ([README §3](../README.md#3-hardware), [`docs/purchase-list.md`](../docs/purchase-list.md), [`docs/bench-materials.md`](../docs/bench-materials.md)) is pending that selection, the 2026-09-26 esp32-s3 pass is superseded into git history (pin `2a6c0af`), and the open lane is the compatible selection and then the co-researcher's purchase-approval sign-off before checkout. The co-researcher asks about hardware and the plan in their own sessions and may propose changes; this rule binds every session, theirs included.
- Before writing any change into the plan (`README.md`, `docs/*`, `.opencode/*`) — a hardware component or material selection, a design decision, a schedule shift, anything — evaluate it against the baseline first: read the touched sections at the baseline (`git show 91148818d416f94080e0e6a0de6531ff67d941d1:README.md`, or `git diff 91148818d416f94080e0e6a0de6531ff67d941d1 -- <paths>`), then trace what the change breaks across the **entire thesis project**, not only the system. Dependency chains to check: the phase gates and bench-test matrix (T0–T8) against the thesis writing stages (real bench results feed thesis §4.1 and the pre-oral defended scope); the ≤ 100 ms motion-to-sound budget and the link-hop stage (the link medium is selected with the hardware — open again since the 2026-09-29 clearing); the self-containment rule (no external runtime sources); the mandatory pointer-tracking stack; the purchasing constraints (pre-soldered for P2, buy-once); the evaluation design (third revision — actual VI participants, within-subject one-visit design, no fallback); and the defense-anchored schedule (pre-oral Fri Dec 11, 2026; final Tue Apr 20, 2027).
- State the impact precisely and honestly: what **breaks**, what merely **degrades**, and which named plan sections absorb it. Be causal, not alarmist — e.g., skipping the bench tests does not disable writing the Methodology chapter; it removes the real results that thesis §4.1 and the pre-oral defense are scheduled to carry, shrinking the defended scope per the schedule's contingency ladder. Use the plan's own numbers (dates, thresholds, counts) wherever they exist.
- Present that analysis to the requester and stop. Do not write the change into the plan unless they explicitly instruct it, and do not advertise that they may order a commit.
