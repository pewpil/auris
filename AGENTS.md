# AGENTS.md — Auris

Standing instructions for this repository. Applies to every session.

> **Scope:** §1–§6 are repository-wide. Thesis-writing conventions live separately in [`thesis/AGENTS.md`](thesis/AGENTS.md) and apply when working inside `thesis/`.

---

## 1. Concept

**Auris** is a head-mounted, audio-only navigation aid driven by a laser pointer, for visually impaired users. This section is the canonical concept statement until a development-plan `README.md` is written; at that point the plan becomes canonical for planning content and this section remains the short concept reference.

### 1.1 User

The user is visually impaired, or a blindfolded sighted person replicating a visually impaired person.

### 1.2 Environment

A room containing obstacles — tables, walls, and similar.

### 1.3 User goal

The user is tasked with going from any point in the room to another point in the room.

### 1.4 Navigation aid

The user is equipped with a head-mounted wearable and a pointer. Sound is projected to the wearable's audio apparatus, which the user then uses to aid navigation.

### 1.5 User equipment

Two devices, both 3D-printed around their components.

**Head-mounted wearable** — worn by the user.

1. *Head position and orientation* — the wearable extracts the user's head position and orientation.
2. *Audio* — the wearable carries an audio apparatus that broadcasts sound to the user's ears.

**Pointer** — handheld.

1. *Laser* — the pointer shoots an imaginary/invisible laser toward obstacles.
2. *Button* — the laser fires only while the button is pushed, and stops the moment the button is released.

### 1.6 System goal

The projected sound corresponds to where the laser shot landed **relative to the user's head** — not to the room, and not to the user's body. It varies with two things: **where the laser shot lands (a)** and **where the user is facing (b)**.

Worked example: if (a) is far and to the user's left, the sound is projected faintly on the left (presumably the user is looking straight ahead); otherwise if (a) is not far, it is louder. But if (b) is right, the sound is heard as though it were at the user's back, because the landing point ends up behind them.

The sound is also placed **vertically**: whether the shot lands above, at, or below the user's head level is conveyed (e.g. aiming down a staircase versus at a wall sign at eye height). The pointer is not fixed at chest height — it may be aimed up or down freely, and the placement follows the hit in 3-D.

Orientation is full 3-D — **yaw, pitch, and roll** — for both the head and the pointer. The projected sound must replicate how the laser hit would actually be heard in real life.

### 1.7 Hardware — the wearable's board is pinned

The **wearable's compute board is pinned** and is not open to change:

| Device | MCU |
|---|---|
| **Head-mounted wearable** | **Raspberry Pi 5, 8 GB** |

**The pointer's MCU is deliberately not pinned.** No part is fixed for it here — that selection stays open. What is fixed is the shape of the answer, and the asymmetry between the two devices is deliberate: the wearable must be a Linux single-board computer because it renders audio, runs computer vision alongside that audio, and hosts the Wi-Fi access point, while the pointer must be a compact battery-powered microcontroller doing onboard orientation fusion and time-of-flight ranging, because it is hand-held, runs off its own cell, and sends only data to the wearable. Those differing requirements are what make the two classes differ; no part was chosen in advance of them.

---

## 2. Documentation standard — every function and class is documented

Every function, method, and class written in this repository must carry a documentation comment at its point of definition. This is explicitly requested by the researcher, so it overrides the default minimal-comment style **for definitions only** — incidental inline comments elsewhere stay rare and purposeful.

**Form per language:**

| Language | Form |
|---|---|
| Python | PEP 257 docstring (`""" ... """`) on every module-level function, method, class, and dataclass |
| TypeScript / TSX | JSDoc block (`/** ... */`) on every function, class, component, and non-obvious type |
| Bash / scripts | Header comment block stating purpose, inputs, outputs |

**Content requirements:**

- **Summary** — one clear line in imperative mood ("Compute the filter cutoff for the active band."), never "This function computes ...".
- **Parameters & returns** — meaning, **units**, and **coordinate frame** whenever numeric (e.g. degrees vs radians, meters, which frame a pose is expressed in, expected value ranges).
- **Errors** — what the function raises/throws and under which conditions.
- **Classes & components** — responsibility, ownership/lifecycle of state, and any invariants callers may rely on.
- Write for the next reader; do not restate the signature or narrate obvious lines.

**When editing existing code:** newly written or substantially rewritten functions/classes follow the same rule. Do not expand the diff by retro-documenting unrelated untouched code unless the researcher asks.

---

## 3. Git — the AI never commits by itself

Do **not** run `git commit`, `git merge`, `git rebase`, `git tag`, or any other history-writing command in this repository — in the main repo or in the `thesis/` submodule. Commits are made by the researcher; the AI's job stops at producing a clean working tree.

- **Allowed:** `git status`, `git diff`, `git log`, `git show`, `git stash list` — inspection only. Temporary, message- or checkout-only rewrites are permitted **only when the researcher explicitly orders them in the current session**, and only on unpushed history.
- **After finishing work:** leave edits staged or unstaged as they are, summarize what changed, and let the researcher commit. Never volunteer a commit, never end a task with "shall I commit this?" — just stop.
- **No unrequested pushes, force-flags, branch deletions, or reflog surgery** — and rebase/reword requests must be verified unpushed first.

---

## 4. Verification policy — no hardware bench phase

**There is no hardware bench phase.** No component is assembled on a breadboard for testing, and no breadboard test matrix is executed. Verification of every engineering claim is by **software simulation or component datasheet**, falling back in a fixed order.

### 4.1 Evidence classes

Every verification claim carries an evidence class, taken in this order — take the strongest one available:

| Tag | Meaning |
|---|---|
| **`[SIM]`** | Simulated — the real code (geometry, renderer, tiering, fusion) run on a desktop machine against synthetic inputs, or a circuit simulated in SPICE. |
| **`[BUD]`** | Budgeted — instruction/cycle counts of the compiled code, scaled to the target silicon against vendor cycle tables. |
| **`[PROP]`** | Propagated — the selected parts' datasheet noise, bias, and accuracy figures seeded through the design's error math into distributions (Monte Carlo). |
| **`[CITE]`** | Cited — datasheet or literature carry, used **only where no simulation or budget method exists**. |

**The fallback rule:** where no software method exists for a claim, **the component datasheet is the evidence** — and the claim is labelled as citation-class. Nothing is quietly upgraded from cited to measured.

**The labelling rule:** no quality claim is presented as measured unless it was. A claim ending at `[CITE]` is visibly weaker than one reaching `[SIM]`, and that difference is reported rather than hidden. Thresholds themselves never move; only their provenance is labelled.

### 4.2 In-study measurement

What no simulation can produce is obtained from the **device measuring itself in use**: instrumentation for this stays in the firmware (structured event stream — sensor frames, render timings from the device's monotonic timer, link sequence and tx/rx timestamps, button state, reset reasons — plus a per-update active-source record for the tracking tiers). This yields real latency and tracking-tier numbers on the evaluated hardware during calibration and the user study, rather than on a bench.

### 4.3 Known residual

The tracking tier's **hand-off frequency and non-line-of-sight error distribution cannot be simulated or budgeted.** Only the tier *logic* (switch continuity, clamping policy, fallback ordering) is verifiable ahead of the build; the *behaviour* rests on in-use telemetry. This is the project's largest evidence gap and is to be stated as a named limitation, not left implicit.

---

## 5. Markdown documents — never artificially wrap lines

Write every prose paragraph as **one single unwrapped line**. Do not hard-wrap prose at a column width, and do not insert line breaks inside a paragraph — neither soft returns nor trailing-space hard breaks — to control how the source looks.

Separate blocks with blank lines, and let each block own its line structure:

- **Paragraphs** — one line each, separated by a blank line. No mid-paragraph returns.
- **Lists** — one item per line; continuation lines run long rather than aligning under the marker.
- **Tables** — one row per line; long cells stay on their row.
- **Headings, block quotes, fenced code** — keep their own structure. This is document structure, not wrapping, and is expected.

The reason is how these files are read: they are consumed in Obsidian, where soft-wrapped source lines are noise and every paragraph renders as a single block regardless. A wrapped paragraph is strictly worse to read in the editor than an unwrapped one, and no line-length convention is worth that cost here.

---

## 6. Side notes in markdown comments

The researcher leaves side notes for the AI inside markdown documents as HTML comments — the syntax is deliberately *not* written out literally here, so that this section cannot itself be mistaken for a pending note. They are invisible in Obsidian's reading view, which is how the researcher reads these files, so a pending note costs nothing to leave sitting in the source.

- **Every HTML comment in a markdown file in this repository is an instruction to carry out, not content for a reader.** Read the comments before editing a document, act on each one, and report what each note was and what it changed.
- **Delete each comment once it has been acted on.** A side note is a request, not a record. Once fulfilled it is stale, and a leftover comment is worse than no comment at all.
- **Provisional text does not need a side note.** When a passage is provisional pending a future decision, the AI records its own removal condition as a bullet in this section, and the passage itself needs no HTML comment. Outstanding conditions are listed below. The record is temporary in exactly the way a comment is: it is deleted once its condition is met, in the same pass that settles the passage.
- **Outstanding provisional passages, and what retires each.** These are the AI's own notes to itself, kept here rather than cluttering the sections they refer to. **When a condition is met, act on it and delete this record in the same pass** — the record is a bookmark, not a log, so a fulfilled entry left behind is as stale as a fulfilled comment:
  - **§1.7's pointer-MCU paragraph, its heading, and the table above it.** The paragraph is provisional, and the heading *"Hardware — the wearable's board is pinned"* is already half a claim about the pointer. When the pointer MCU is settled: delete the paragraph, retitle the section to cover both devices, and fold the pointer into the table.
- **A note that cannot be acted on yet stays until it can.** Some notes are triggers rather than tasks — *"delete this paragraph once the pointer MCU is pinned"* is not actionable while that selection is still open. Such a note is kept, and deleted at the moment its condition is met; the delete-on-completion rule above applies once the condition is satisfied. Removing it early loses the only record of what still has to be revisited.
- **Superseded text goes with the note that retired it.** When a provisional passage is settled and replaced, delete the old passage as well — the point of the note is that the provisional text is temporary, and it should not outlive the decision it was waiting on.
- **A comment never overrides the live conversation.** If a comment contradicts what the researcher says in the current session, the spoken instruction wins — carry out the spoken one, leave the comment in place, and say what conflicted rather than picking one silently.
- **A comment whose intent is ambiguous gets flagged, not guessed.** State the reading you would act on and what is uncertain, then let the researcher confirm.
- **Only HTML comments are side notes.** Ordinary prose in the document is content and is never treated as an instruction.
