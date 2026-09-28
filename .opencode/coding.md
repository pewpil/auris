# Auris — Coding instructions (coding.md)

Standing instructions for **writing and editing code** in this project. This
file is wired into `.opencode/opencode.jsonc` → `instructions`, so every
session loads it alongside the other instruction files.

> **Scope rule:** applies to every code change in this repository — any
> language, shell scripts, and configs. Project & scheduling rules live in
> `instruction.md`; thesis-writing rules in `man.md`.

## 1. Documentation standard — every function and class is documented

Every function, method, and class the AI writes must carry a documentation
comment at its point of definition. The AI is explicitly asked for this by the
researcher, so this rule overrides the default minimal-comment style **for
definitions only** — incidental inline comments elsewhere still stay rare and
purposeful.

**Form per language:**

| Language | Form |
|---|---|
| Python | PEP 257 docstring (`""" ... """`) on every module-level function, method, class, and dataclass |
| TypeScript / TSX | JSDoc block (`/** ... */`) on every function, class, component, and non-obvious type |
| Bash / scripts | Header comment block stating purpose, inputs, outputs |

**Content requirements:**

- **Summary** — one clear line in imperative mood ("Compute the filter
  cutoff for the active band."), never "This function computes ...".
- **Parameters & returns** — meaning, **units**, and **coordinate frame**
  whenever numeric (e.g., degrees vs radians, meters, which frame a pose is
  expressed in, expected value ranges).
- **Errors** — what the function raises/throws and under which conditions.
- **Classes & components** — responsibility, ownership/lifecycle of state,
  and any invariants callers may rely on.
- Write for the next reader; do not restate the signature or narrate obvious
  lines.

**When editing existing code:** newly written or substantially rewritten
functions/classes follow the same rule. Do not expand the diff by
retro-documenting unrelated untouched code unless the researcher asks.

## 2. Git — the AI never commits by itself

The AI does **not** run `git commit`, `git merge`, `git rebase`, `git tag`,
or any other history-writing command in this repository — in the main repo
or in the `thesis/` submodule. Commits are made by the researcher; the AI's
job stops at producing a clean working tree.

- **Allowed:** `git status`, `git diff`, `git log`, `git show`, `git stash
  list` — inspection only. Temporary, message- or checkout-only rewrites are
  permitted **only when the researcher explicitly orders them in the current
  session** (e.g. "remove that commit body"), and only on unpushed history.
- **After finishing work:** leave edits staged or unstaged as they are,
  summarize what changed, and let the researcher commit. Never volunteer a
  commit, never end a task with "shall I commit this?" — just stop.
- **No unrequested pushes, force-flags, branch deletions, or reflog
  surgery** — and rebase/reword requests must be verified unpushed first.
