# AI_HANFOFF - Continuous Sprint Handoff

## Purpose

This file is the operational contract for any AI agent that works on MyOS.  
It must be updated at the end of every work session so the next agent can continue without losing context.

## Project objective

Build a fully functional multilingual operating system with:

- Multi-architecture support (x86 first, ARM second, RISC-V later).
- Strong kernel fundamentals (boot, memory, processes, syscalls, storage, networking, security).
- CLI and full GUI desktop stack.
- Language-aware command and message system as a first-class feature.
- Production mindset: testability, maintainability, release strategy.

## Non-negotiable operating rules for agents

1. Never delete or rewrite unrelated user changes.
2. Keep architecture-specific and generic code separated.
3. Prioritize small, testable increments over large rewrites.
4. Update both roadmap and handoff before ending a session.
5. Do not claim completion without verification evidence.
6. Keep multilingual parity when adding user-facing functionality.
7. Treat security as a default requirement, not an optional phase.
8. Prefer fixing root causes over temporary patches.
9. If blocked, document blocker + next best action clearly.
10. Preserve bootability at all times.
11. Prefer ASM for hardware-near code paths (boot, vectors, trap entry, context switch, MMU/CPU control).
12. Do not replace ASM-critical paths with high-level code unless explicitly approved.

## Session workflow (must follow every time)

1. Read `GENERAL_ROADMAP.md`.
2. Read this `AI_HANFOFF.md`.
3. Confirm current sprint goal and task scope.
4. Execute only scoped tasks unless user explicitly expands scope.
5. Run build + validation checks relevant to changed areas.
6. Update sections:
   - "Current sprint state"
   - "Work completed this session"
   - "Open blockers"
   - "Next session startup checklist"
7. Mark roadmap checkboxes that were completed.

## Quality gate checklist (before ending session)

- [ ] Build succeeds for the target architecture profile used in this session.
- [ ] Boot path still works in emulator.
- [ ] No new obvious regressions in touched subsystem.
- [ ] Any new command/message has multilingual mapping.
- [ ] Documentation updated (`GENERAL_ROADMAP.md` and this file).
- [ ] Remaining risks and follow-ups documented.

## Current sprint state

- Sprint ID: S1
- Sprint goal: Close M1 baseline and validate ARM runtime/IRQ path with scheduler-tick preparation.
- Scope:
  - Ensure `arch.h` interface parity across active/planned architectures.
  - Bring ARM to stable boot + IRQ/timer heartbeat in QEMU.
  - Wire ARM timer IRQ to shared scheduler tick entry (compat mode).
  - Update roadmap checkboxes and handoff notes.
- Status: IN PROGRESS
- Primary architecture focus: ARM
- Secondary architecture focus: x86 maintenance

## Work completed this session

- Read all existing markdown documents in repository.
- Removed previous markdown planning docs as requested.
- Created `GENERAL_ROADMAP.md` with comprehensive milestone checklist (>100 tasks).
- Created `AI_HANFOFF.md` with operating rules and sprint continuity protocol.
- Established project-wide definition of done and session quality gate.
- Added missing `arch_send_ipi()` stubs for RISC-V and MIPS architecture backends.
- Verified x86 build/image generation with `make all` after multi-arch interface updates.
- Marked completed M1 tasks in `GENERAL_ROADMAP.md`.
- Fixed ARM vectors return syntax and alignment requirements.
- Added ARM exception-mode stack initialization in boot ASM.
- Stabilized ARM runtime path with UART boot logs, GIC/timer IRQs, and heartbeat.
- Added ARM timer tick hook and connected it to shared `scheduler_tick()` via compatibility linkage.
- Extended ARM linker object set (`scheduler.c`, `node.c`, `kstring.c`) with ARM compatibility stubs.
- Connected `context_switch()` compatibility path to real ARM ASM routine `context_switch_asm`.
- Verified ARM remains boot-stable after scheduler-core linkage (`make run-arm` + heartbeat).

## Current known blockers

- No immediate blocker for M1 baseline closure.
- Active risks:
  - ATA stability (`block_init` crash history).
  - ARM process/context subsystem remains partial (compat stubs still in use for `process_get_by_pid` and `process_terminate`).
  - GUI integration footprint vs bootloader kernel size.

## Next session startup checklist

1. Re-read `GENERAL_ROADMAP.md` and this file.
2. Select one focused milestone slice (recommended: ARM process/context bring-up to replace compat stubs).
3. Confirm impacted files before modifying code.
4. Implement smallest boot-safe increment.
5. Build and run emulator verification.
6. Update roadmap checkboxes for finished tasks.
7. Append concise session log below.

## Session log template (append-only)

Use this exact template:

```text
### Session YYYY-MM-DD HH:MM (local)
- Goal:
- Scope:
- Files changed:
- Validation run:
- Result:
- Roadmap tasks checked:
- Blockers:
- Next recommended step:
```

## Active decisions and conventions

- Keep command aliases language-driven and centralized.
- Keep user-visible strings out of hardcoded command logic.
- Preserve compatibility with current shell behavior while extending capabilities.
- Favor additive architecture abstractions first, then refactors.
- Every new subsystem must define:
  - Public interface
  - Invariants
  - Failure behavior
  - Validation plan

## Ownership map (rolling)

- Boot and arch layer: unassigned (next session should assign).
- Memory and process core: unassigned.
- Filesystem and storage: unassigned.
- Networking: unassigned.
- GUI stack: unassigned.
- Localization system: unassigned.
- Tooling and CI: unassigned.

## Exit criteria for Sprint S1

- [x] Roadmap exists as single source of truth.
- [x] Handoff process exists and is explicit.
- [x] Next sprint focus selected and recorded.
- [x] First implementation sprint (S1) kickoff prepared.
- [x] `arch.h` parity completed for x86/ARM/RISC-V/MIPS.
- [x] x86 build validated post-change.

### Session 2026-05-06 12:59 (local)
- Goal: Start first implementation sprint after planning setup.
- Scope: M1 baseline alignment and architecture interface parity.
- Files changed: `arch/riscv/kernel/core/arch.c`, `arch/mips/kernel/core/arch.c`, `GENERAL_ROADMAP.md`, `AI_HANFOFF.md`.
- Validation run: `make all`.
- Result: Success (disk image generated, no build break from new arch stubs).
- Roadmap tasks checked: M1 arch API and x86/ARM abstraction checklist items.
- Blockers: ARM runtime path not validated in this session.
- Next recommended step: Run `make run-arm` and implement early UART output milestone.

### Session 2026-05-06 13:45 (local)
- Goal: Validate real ARM runtime path and hook scheduler tick.
- Scope: ARM vectors/IRQ/timer bring-up, runtime heartbeat, scheduler tick wiring.
- Files changed: `arch/arm/boot/boot.S`, `arch/arm/kernel/core/vectors.S`, `arch/arm/kernel/core/arch.c`, `arch/arm/kernel/core/arm_irq.c`, `arch/arm/kernel/core/arm_main.c`, `arch/arm/kernel/core/sched_compat.c`, `Makefile`, roadmap/handoff docs.
- Validation run: `make build/arm/kernel.elf`, `make run-arm`.
- Result: Success (ARM boots, IRQ path active, timer heartbeat visible, scheduler tick hook connected).
- Roadmap tasks checked: ARM MMU placeholder, ARM UART early console, ARM IRQ end-to-end validation, scheduler tick hook integration.
- Blockers: Full ARM process/context implementation still pending (compat stubs currently used).
- Next recommended step: Port ARM process/context primitives and remove `sched_compat.c` stubs.

### Session 2026-05-06 13:46 (local)
- Goal: Advance ARM scheduler integration while preserving boot stability.
- Scope: Replace no-op context switch compatibility with real ARM ASM context switch path.
- Files changed: `arch/arm/kernel/core/sched_compat.c`, `GENERAL_ROADMAP.md`, `AI_HANFOFF.md`.
- Validation run: `make run-arm`.
- Result: Success (ARM boot/logs unchanged, heartbeat maintained, scheduler compatibility layer now uses ASM switch).
- Roadmap tasks checked: ARM scheduler linkage tasks in M3.
- Blockers: Remaining compatibility stubs (`process_get_by_pid`, `process_terminate`) still pending real ARM process layer.
- Next recommended step: Implement minimal ARM process table + `process_get_by_pid` to remove first remaining stub.
