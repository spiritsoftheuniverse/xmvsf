# Development workflow — no GitHub Actions

GitHub is used for source control, history, branches, and recovery. Development and code review may happen in ChatGPT using the GitHub connection, or directly in the local checkout. Do **not** use GitHub Actions for builds, tests, monitoring, or routine validation.

## Workflow

1. Inspect the current branch, source, and existing logs before changing code. Preserve local changes and avoid overwriting newer local work.
2. Make the smallest source changes necessary and record them in a normal Git commit or patch. Do not use Actions as an edit, build, or validation mechanism.
3. Run native Windows builds and runtime tests on the development computer using the existing project launchers; ChatGPT's GitHub access alone cannot execute those local Windows programs.
4. Keep build transcripts, runtime logs, and replay/test evidence. Mark a change **unverified** until its actual results have been inspected; compilation is not proof of gameplay correctness.
5. Never overwrite a protected Stable build as a consequence of an unverified change. Do not remove source, warm build caches, logs, or working data just to make validation pass.
6. Use Git history, branches, and commit identifiers to recover or compare revisions. Keep a concise change summary and mention unresolved regressions in each handoff.

## GitHub Actions

Existing Actions YAML is retained verbatim under `.github/workflows-archive/` for historical reference. No active YAML remains under `.github/workflows/` on `main`. Do not move these files back or add new active workflow files without an explicit request.

## Project-specific validation

- Canonical build/run entry points: `BUILD_TEST.bat`, `RUN_TEST.bat`.
- Keep the existing replay runner (`RUN_REPLAY.bat`) and recording / diagnostic infrastructure intact.
- Validate DHC entry/exit gating and successful supers with actual replay evidence, not guessed state addresses. Protect training/versus, stage camera, Classic/Tournament rules, and F9 replay behavior against regression.
- The source and warm local buildtree take precedence over generated handoff archives. Never overwrite Stable during test builds.
