# Current Milestone Lock

- Active milestone: `18`
- Title: `Headless Runtime Frame-Dump Parity`
- Status: `accepted`
- Locked on plan: `docs/emulator/07-implementation-plan.md`
- Contract file: `docs/emulator/milestones/18.md`

Execution rules:
- Only milestone `18` task files present in `docs/tasks/active/` may be
  executed.
- Milestone `17` is accepted.
- Keep milestone `18` work inside `src/frontend/`, `tests/`, and `docs/` as
  allowed by the milestone contract.
- Milestone `18` is the narrow headless/runtime parity pass so `--dump-frame`
  reflects the active ROM session rather than the built-in fixture image.
- Milestone `18` was accepted after human review of runtime `--dump-frame`
  parity for both `256x212` and `512x212` output plus the explicit
  fixture-only dump path.
