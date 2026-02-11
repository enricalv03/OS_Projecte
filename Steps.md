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