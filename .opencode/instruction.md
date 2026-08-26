# Auris — Standing Instructions & Standards

General rules that apply to every session on this project.

## Section referencing (§)

- The **section symbol (§)** belongs to **thesis-paper writing convention**. It is used when referencing other sections **within the thesis paper**.
- Inside the thesis paper, every cross-reference to another section must use **both** the § symbol **and** a hyperlink to that section (clickable in working drafts).
- Repository documents (`README.md`, `.opencode/*`, `docs/*`) may keep § references for readability, but they must always be written as **clickable anchor links** using GitHub's native kebab-case heading slugs — e.g. `[§7.2](#72-prototype-hardware-purchase-approval)` — never as bare unlinked text. (GitHub slug rule: lowercase, spaces → hyphens, punctuation dropped; `&` leaves a double hyphen.) This repo is read on GitHub, so GitHub-style slugs are the standard.

## Thesis-paper link stripping (future task)

- Every form of linking in the thesis paper — section self-references **and** outside resources alike — **will be stripped from the final submitted version**.
- Before final formatting / submission, **remind the user** to strip all hyperlinks from the paper while keeping the plain-text § numbers. (Standing pointer also kept in `.opencode/plan.md`.)
