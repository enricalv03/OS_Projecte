# GENERAL ROADMAP - MyOS

## Vision

Build a production-grade, multilingual, multi-architecture operating system that can run on x86, ARM, and later RISC-V, with both CLI and GUI environments, a robust security model, networking, storage, drivers, developer tooling, and release engineering.

## Rules of execution

- Work in small vertical slices that boot and run in QEMU first.
- Keep architecture-specific code isolated under `arch/<arch>/`.
- Keep core generic logic architecture-agnostic under `kernel/` and `fs/`.
- Maintain language parity for all user-visible commands/messages.
- Every task must include tests or validation steps.
- No large refactors without updating this roadmap.
- ASM-first policy for low-level paths: boot, mode switching, vectors/interrupt stubs, context switch, trap/syscall entry, and MMU register programming.
- C (or C++) may wrap ASM only when it improves portability/readability without hiding hardware behavior.

## Target repository structure (final intent)

```text
arch/
  x86/
  arm/
  riscv/                     (future)
kernel/
  core/
  mm/
  sched/
  ipc/
  security/
  syscalls/
  drivers/
  net/
  gui/
  cxxrt/
fs/
  vfs/
  ramfs/
  diskfs/
lib/
  klib/
user/
  init/
  shell/
  utils/
assets/
  fonts/
  images/
  themes/
tools/
docs/
tests/
```

## Milestone checklist

### M0 - Governance and baseline

- [ ] Define coding standards (C, ASM, C++).
- [ ] Define branch strategy (`main`, `dev`, `feature/*`).
- [ ] Define semantic versioning rules.
- [ ] Add contributor workflow and review checklist.
- [ ] Add issue templates (bug, feature, architecture proposal).
- [ ] Add PR template with test evidence section.
- [ ] Add architecture decision records (ADR) directory.
- [ ] Add reproducible toolchain setup docs.
- [ ] Add deterministic build requirements.
- [ ] Define supported emulator matrix and versions.

### M1 - Boot and architecture abstraction

- [ ] Finalize x86 boot flow (MBR -> stage2 -> kernel entry).
- [ ] Expand kernel load size strategy (beyond 120 sectors).
- [ ] Add runtime kernel image integrity check.
- [x] Define `arch_api.h` for generic kernel calls.
- [x] Implement x86 `arch_enable_interrupts`.
- [x] Implement x86 `arch_disable_interrupts`.
- [x] Implement x86 `arch_halt`.
- [x] Implement x86 `arch_idle`.
- [ ] Implement x86 timer abstraction hook.
- [ ] Implement x86 IRQ acknowledge abstraction.
- [x] Add ARM boot skeleton in `arch/arm/boot/`.
- [x] Add ARM exception vector skeleton.
- [x] Add ARM MMU bootstrap placeholder.
- [x] Add ARM timer abstraction stub.
- [x] Add ARM UART early console stub.
- [x] Add cross-arch build flag (`ARCH=x86|arm`).
- [x] Add arch-specific linker selection.
- [ ] Add per-arch kernel entry contract doc.
- [ ] Add architecture capability matrix document.
- [ ] Define future RISC-V port interface expectations.
- [x] Validate ARM IRQ path end-to-end (vectors + GIC + timer + idle loop heartbeat).
- [x] Wire ARM timer tick hook to shared scheduler tick path (compatibility mode).

### M2 - Memory management

- [ ] Harden PMM bitmap initialization checks.
- [ ] Add PMM stress test command.
- [ ] Implement kernel heap fragmentation metrics.
- [ ] Add slab allocator prototype for kernel objects.
- [ ] Add virtual memory map introspection command.
- [ ] Harden page fault handler diagnostics.
- [ ] Implement copy-on-write groundwork structures.
- [ ] Add userspace address space abstraction.
- [ ] Add `mmap` region bookkeeping improvements.
- [ ] Add guard pages for kernel stacks.
- [ ] Add memory leak tracker in debug builds.
- [ ] Add `kmalloc` alignment guarantees documentation.
- [ ] Add per-process memory usage accounting.
- [ ] Add OOM handler policy draft.
- [ ] Add memory subsystem test suite.

### M3 - Process, scheduler, IPC

- [ ] Refactor PCB structure into arch-neutral + arch-context parts.
- [ ] Implement robust context save/restore checks.
- [x] Integrate ARM timer IRQ with shared `scheduler_tick()` (initial compatibility mode).
- [x] Link ARM build with shared scheduler core (`scheduler.c` + `node.c`) in QEMU path.
- [x] Replace ARM scheduler compatibility stubs with full process/context implementation (early bring-up subset: sleep/wake + signal stubs + non-weak context switch).
- [x] Bring up first runnable ARM kernel thread (`process_create_kernel_thread`) with cooperative yields.
- [x] Validate short-lived ARM thread lifecycle (`spawn -> run -> process_exit -> reap -> slot reuse`) under scheduler load.
- [ ] Add scheduler policy abstraction layer.
- [ ] Keep round-robin as baseline policy.
- [ ] Add priority scheduler experimental mode.
- [ ] Add scheduler starvation detector.
- [ ] Add per-process runtime statistics.
- [ ] Add kernel threads API.
- [ ] Add init process contract.
- [ ] Add process states audit (`new`, `ready`, `running`, `blocked`, `zombie`).
- [x] Implement wait queues foundation.
- [x] Implement pipe IPC baseline.
- [ ] Implement message queue IPC baseline.
- [ ] Implement signal delivery baseline.
- [ ] Add process debugger command set.

### M4 - Syscalls and user mode

- [ ] Freeze syscall numbering policy.
- [ ] Generate syscall table from single source file.
- [ ] Add syscall argument validation layer.
- [ ] Add userspace pointer safety checks.
- [ ] Add syscall tracing in debug mode.
- [ ] Add syscall latency instrumentation.
- [ ] Implement user mode entry path hardening.
- [ ] Add context switch to ring3 validation tests.
- [ ] Add minimal libc-like userspace headers.
- [ ] Add static userspace app loader.
- [ ] Add ELF loader phase 1 (basic segments).
- [ ] Add execve-compatible path resolution model.
- [ ] Add compatibility notes for future POSIX subset.
- [ ] Add syscall ABI docs per architecture.
- [ ] Add syscall fuzzing harness (kernel-side).

### M5 - Filesystems and storage

- [ ] Split VFS into dedicated module tree.
- [ ] Harden path normalization (`.`, `..`, multiple slashes).
- [ ] Add mount table implementation.
- [ ] Add root filesystem mount workflow.
- [ ] Add open file descriptor table refactor.
- [ ] Add file permission metadata model.
- [ ] Add ownership metadata model (`uid`, `gid`).
- [ ] Add link count support in VFS node model.
- [ ] Add symbolic link support (phase 1).
- [ ] Add hard link support (phase 1).
- [ ] Add journaling design draft for diskfs.
- [ ] Add diskfs consistency checker tool.
- [ ] Add buffered block cache layer.
- [ ] Add writeback and flush policies.
- [ ] Add filesystem stress tests (create/read/delete loops).

### M6 - Drivers and hardware enablement

- [ ] Stabilize keyboard driver event queueing.
- [ ] Stabilize mouse driver packet parsing.
- [ ] Add generic input subsystem abstraction.
- [ ] Add ATA driver crash root-cause fix.
- [ ] Add block device registration framework.
- [ ] Add partition parser support (MBR, GPT phase 1).
- [ ] Add PCI enumeration baseline.
- [ ] Add driver probe and bind lifecycle.
- [ ] Add serial driver improvements for diagnostics.
- [ ] Add framebuffer mode abstraction.
- [ ] Add timer driver abstraction for multiple sources.
- [ ] Add ACPI parser phase 1 (x86).
- [ ] Add device tree parser phase 1 (ARM).
- [ ] Add power management hooks baseline.
- [ ] Add driver test harness in emulator.

### M7 - Networking

- [ ] Define `net_device` interface.
- [ ] Add Ethernet frame parser.
- [ ] Add ARP cache and ARP request/reply handling.
- [ ] Add IPv4 packet parsing and routing baseline.
- [ ] Add ICMP echo request/reply.
- [ ] Add UDP sockets phase 1.
- [ ] Add TCP state machine phase 1.
- [ ] Add loopback interface support.
- [ ] Add socket syscall extension set.
- [ ] Add DNS resolver userspace utility.
- [ ] Add DHCP client baseline.
- [ ] Add e1000 driver phase 1 for QEMU.
- [ ] Add packet capture debug command.
- [ ] Add network throughput benchmark tool.
- [ ] Add network stack fuzz and malformed packet tests.

### M8 - Security

- [ ] Define kernel threat model document.
- [ ] Define secure coding checklist for kernel changes.
- [ ] Add credential model (`uid`, `gid`, groups).
- [ ] Add permission checks in VFS operations.
- [ ] Add process capability model phase 1.
- [ ] Add syscall allow/deny audit hooks.
- [ ] Add stack canary support for kernel builds.
- [ ] Add basic ASLR research/prototype note.
- [ ] Add kernel panic redaction policy for secrets.
- [ ] Add random number subsystem baseline.
- [ ] Add secure boot investigation for future hardware.
- [ ] Add audit logging subsystem phase 1.
- [ ] Add failed login throttling concept.
- [ ] Add security test scenarios checklist.
- [ ] Add vulnerability triage process in docs.

### M9 - Multilingual system (core differentiator)

- [ ] Define language pack format for commands/messages.
- [ ] Define stable message key naming conventions.
- [ ] Migrate all shell output to message keys.
- [ ] Migrate all kernel user-facing strings to message keys.
- [ ] Add English pack validation.
- [ ] Add Spanish pack validation.
- [ ] Add Catalan pack validation.
- [ ] Add missing command alias parity checks.
- [ ] Add runtime language switch atomicity check.
- [ ] Add fallback behavior when translation key missing.
- [ ] Add localization test command (`langtest`).
- [ ] Add language pack linter tool.
- [ ] Add UTF-8 strategy document and constraints.
- [ ] Add right-to-left language feasibility study.
- [ ] Add contribution guide for new languages.

### M10 - CLI, shell, and user tools

- [ ] Refactor shell parser for quoting and escaping.
- [ ] Add environment variables support.
- [ ] Add shell scripting basics (batch execution).
- [ ] Add command history persistence model.
- [ ] Add job control phase 1.
- [ ] Add `find` command implementation.
- [ ] Add `grep` command implementation.
- [ ] Add `ps` command improvements.
- [ ] Add `top`-like live process monitor.
- [ ] Add `netstat` command.
- [ ] Add package-like app installer concept draft.
- [ ] Add shell completion engine phase 1.
- [ ] Add command permission model integration.
- [ ] Add CLI UX consistency audit across languages.
- [ ] Add shell integration tests.

### M11 - GUI and desktop stack

- [ ] Finalize compositor architecture document.
- [ ] Add robust double-buffering in framebuffer pipeline.
- [ ] Add font subsystem (PSF) with cache.
- [ ] Add image loader (BMP phase 1).
- [ ] Add renderer primitives (rect, line, text, alpha blend).
- [ ] Add window object model.
- [ ] Add workspace manager.
- [ ] Add input focus management.
- [ ] Add panel/status bar baseline.
- [ ] Add launcher baseline.
- [ ] Add widget toolkit core (`Widget`, `Window`, `Button`, `Textbox`).
- [ ] Add terminal GUI app baseline.
- [ ] Add file manager GUI app baseline.
- [ ] Add text editor GUI app baseline.
- [ ] Add settings GUI app baseline.
- [ ] Add theme engine and configuration format.
- [ ] Add animation engine phase 1.
- [ ] Add compositor damage tracking.
- [ ] Add GUI language switch integration.
- [ ] Add GUI regression test plan.

### M12 - Build, CI, quality, and observability

- [ ] Split debug and release build profiles.
- [ ] Add warnings-as-errors policy for CI.
- [ ] Add static analysis target in CI.
- [ ] Add emulator smoke tests in CI.
- [ ] Add boot log artifact export in CI.
- [ ] Add kernel unit-test harness where feasible.
- [ ] Add integration test matrix (x86 variants).
- [ ] Add architecture nightly build (x86 + ARM).
- [ ] Add performance benchmark suite.
- [ ] Add boot-time regression tracking.
- [ ] Add kernel tracing ring buffer.
- [ ] Add panic dump format and parser tool.
- [ ] Add changelog generation automation.
- [ ] Add release packaging script.
- [ ] Add binary reproducibility verification.

### M13 - Distribution, docs, and release

- [ ] Define release channels (`nightly`, `beta`, `stable`).
- [ ] Add installation guide for VM usage.
- [ ] Add installation guide for bare metal (advanced).
- [ ] Add hardware support table.
- [ ] Add known limitations table.
- [ ] Add roadmap status dashboard section.
- [ ] Add architecture internals handbook.
- [ ] Add driver development handbook.
- [ ] Add localization handbook.
- [ ] Add security response policy.
- [ ] Add long-term support policy.
- [ ] Add telemetry policy (if enabled in future).
- [ ] Add public demo checklist.
- [ ] Tag and publish v0.1.0 milestone.
- [ ] Define v1.0.0 acceptance criteria.

## Definition of done (applies to each task)

- [ ] Code implemented and reviewed.
- [ ] Build passes in target architecture profile.
- [ ] Runtime validation performed in QEMU (and hardware when applicable).
- [ ] Related docs updated.
- [ ] Roadmap checkbox updated.
- [ ] Handoff status updated in `AI_HANFOFF.md`.

## Active sprint placeholder

- Sprint goal: Stabilize ARM runtime baseline and prepare process/context integration.
- Start date: 2026-05-06
- End date: TBD
- Owner: AI + user pair-programming
- Blockers: Full ARM process/context subsystem still stubbed for shared scheduler linkage.
- Exit criteria: ARM boots reliably, IRQ timer tick runs continuously, scheduler tick hook active, and compatibility stubs are being phased out.
