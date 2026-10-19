# Copilot prompts

This folder keeps a copy of every instruction given to GitHub Copilot, starting with Stage 2. It means the project's history shows not only **what** code was written, but exactly **what Copilot was asked to do** at each step.

## How it is used

- The owner saves each prompt here **before** giving it to Copilot.
- Files are named by stage and sub-stage, for example:
  - `stage_2a_prompt.txt` — the main instruction for sub-stage 2a;
  - `stage_2a_followup_1.txt` — a later request in the same sub-stage (a fix, a comments pass, and so on).
- Each prompt is committed together with the work it produced.
- These are owner documents. Copilot only reads them and never edits them (rule 2b in `.github/copilot-instructions.md`).

## Earlier stages

Prompts given before Stage 2 were not saved as files. Their content is summarised in the Stage 0 and Stage 1 records (`docs/stages/`), Section 9 of `STAGE_00.md` and Section 12 of `STAGE_01.md`, and in Copilot's handoff notes (`results/notes_from_github_copilot/`).

Decision record: D-013 in `docs/DECISIONS.md`.
