The Learning Path for Building a Real OS
Building toward a full OS like Ubuntu or Arch, here's the logical order that builds on what you already know:

Phase 1: Core Hardware Interface (Next Steps)
Keyboard Input ← Start here (builds on your text output)
Interrupt Handling (how OS responds to hardware)
Timer/Clock (scheduling foundation)

Phase 2: Memory & Process Management
Memory Management (paging, virtual memory)
Basic Process Management (running multiple programs)
System Calls (how programs talk to OS)

Phase 3: File System & Storage
Disk I/O (reading/writing files)
Basic File System (organizing files)

Phase 4: Advanced Features
Networking (connecting to internet)
Graphics/GUI (visual interface)
Device drivers (supporting hardware)


There are exactly 256 possible interrupts (0-255):
0-31: CPU exceptions (divide by zero, page fault, etc.)

32-47: Hardware interrupts (keyboard, timer, mouse, etc.)

48-255: Software interrupts (system calls, etc.)

Keyboard is interrupt #33 (0x21 in hex)


RUN steps:
  docker run -it -v "/Users/enric/Documents/UNI Informatica/Random1":/workspace osdev  (In every session has to be ran)
  make all


Processos:
Què falta (TODOs per més endavant)
Copy kernel mappings: necessita GDT per user mode (segments d'usuari)
Free memory properly: necessita més gestió de memòria per processos
Notify parent: necessita system calls per comunicació entre processos
User mode: necessita segments GDT per executar codi d'usuari

Següent pas:
Les system calls bàsiques estan funcionals. Per completar el sistema, cal:
File System — per implementar exec, read, write
Signals — per implementar wait, sleep
Fork — per clonar processos

Okay, I'm building an OS from scratch. The main purposes are several:
1. Bare-metal multi-architecture support. The OS must run natively on any architecture — x86-64 (Intel/AMD), ARM (including AArch64 for modern ARM boards and Apple Silicon class chips), and with the design open enough to be ported to RISC-V or MIPS down the line. This means the architecture-specific code (bootloader, interrupt handling, memory management, context switching) must be cleanly isolated behind abstraction layers so the upper layers of the kernel don't care what hardware they're sitting on.
2. Multikernel design. Rather than a single monolithic kernel image, the OS is structured as a multikernel — each CPU core (or logical processing unit) runs its own kernel instance with its own state, and cores communicate via explicit message passing rather than shared memory locks. This approach scales better on modern multi-core and heterogeneous hardware, and maps well to a bare-metal multi-architecture target since each architecture port can behave as a node in the multikernel mesh.
3. Multi-language OS. When a language switch is performed at runtime, all system strings, UI labels, error messages, and kernel-facing text interfaces must swap atomically to the target language set. This isn't just a locale layer on top — the language context is a first-class OS concept, meaning the ABI for system calls involving text must account for encoding and language context from the ground up.
4. Full UNIX-like functionality. The OS should reach feature parity with a modern Linux-class system: process scheduling, virtual memory, a VFS layer, networking stack, device drivers, and a proper POSIX-compatible syscall surface. It will support useful kernels beyond just the base one — including a microkernel personality for isolated services, and optionally a unikernel mode for running single-purpose workloads directly on the hardware with zero overhead.
5. Graphical stack in C, Hyprland-inspired. The entire graphical layer — compositor, window manager, rendering pipeline — will be written in C. The visual design philosophy draws from Hyprland: smooth animations, tiling with floating support, and a modern Wayland-like protocol for client-window communication, but implemented from scratch without pulling in existing display server code.
6. Primary implementation in ASM and C. The core kernel, bootloader, and hardware abstraction layers are written in assembly (architecture-specific) and C (portable layer). No higher-level languages in the kernel proper. The goal is deep understanding at each step — so any code changes exist only to correct or verify; full code blocks are provided inline so each step can be read, understood, and typed by hand if desired.