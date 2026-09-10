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
