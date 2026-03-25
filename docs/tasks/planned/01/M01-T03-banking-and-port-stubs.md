# M01-T03 — Add Banking Control and Stubbed Port Endpoints

Status: `planned`
Milestone: `1`
Depends on: `M01-T02`

Implements against:
- `docs/spec/00-overview.md`
- `docs/spec/04-io.md`
- `docs/emulator/03-cpu-and-bus.md`

Scope:
- Implement the banked ROM window control surface required by the hardware map.
- Add stub peripheral endpoints for all documented I/O ports.
- Keep all endpoints non-functional beyond routing and placeholder responses.

Done when:
- Bank switching affects the `0x4000-0x7FFF` logical window correctly.
- All documented ports have a routed endpoint or explicit open-bus behavior.

Completion summary:
- Pending.
