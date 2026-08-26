# Section 1 — Introduction to ISA

## 1.1 What the ISA Specifies
![ISA components overview](isa-overview.png)

The ISA is the abstract boundary between software and hardware. It defines the capabilities the processor exposes to a programmer.

- **Instructions**: the operations the CPU can perform — arithmetic, logic, branches, and memory access.
- **Data types & formats**: how integers, floats, and addresses are represented and sized.
- **Registers**: the programmer-visible storage inside the CPU, both general and special purpose.
- **Addressing modes**: the ways an instruction locates its operands (immediate, direct, indirect, indexed).
- **Memory model**: how memory is addressed, aligned, and accessed.
- **Exceptions & interrupts**: how the CPU reacts to errors and external events.

## 1.2 Why the ISA Matters
The ISA is a contract: any program built for an ISA runs on any compliant hardware, independent of the microarchitecture.

- Enables **software portability** across implementations.
- Lets hardware evolve (pipelining, caches, superscalar) without breaking programs.
- Standardizes how compilers and operating systems target a machine.

## 1.3 Examples of Common ISAs
- **x86 / x86-64**: dominant in PCs and servers.
- **ARM**: leader in mobile, embedded, and now laptops/cloud.
- **RISC-V**: open, extensible, popular in research and startups.
- **MIPS / PowerPC / SPARC**: historical or niche specialist use.
