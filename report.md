# Instruction Set Architecture (ISA)

This report is divided into three sections, one for each presenter.

---

## Section 1 — Introduction to ISA (Presenter 1)

The **Instruction Set Architecture (ISA)** is the abstract interface between the hardware and the software of a computer. It defines what the processor can do and how the software controls it.

### What the ISA specifies
- **Instructions**: the set of operations the processor can execute (arithmetic, logical, control, memory access).
- **Data types and formats**: the representation of integers, floating-point numbers, and addresses.
- **Registers**: the set of general-purpose and special-purpose registers visible to the programmer.
- **Addressing modes**: how the operands of an instruction are located (immediate, direct, indirect, indexed, etc.).
- **Memory model**: how memory is addressed and accessed.
- **Interrupt and exception handling**: how the processor responds to external events and errors.

### Why the ISA matters
The ISA acts as a contract: software written for a given ISA runs on any processor that implements that ISA, regardless of the underlying microarchitecture. This separation lets the same programs run on different hardware implementations and allows hardware to evolve independently of software.

### Examples of common ISAs
- **x86 / x86-64**: used in most desktop and laptop computers.
- **ARM**: dominant in mobile and embedded devices, and increasingly in servers.
- **RISC-V**: an open, extensible ISA gaining popularity in research and industry.
- **MIPS, PowerPC, SPARC**: other historical and specialized ISAs.

---

## Section 2 — Reduced Instruction Set Computer (RISC) (Presenter 2)

**RISC** is a design philosophy that favors a small, highly optimized set of simple instructions that can execute in a single clock cycle.

### Core principles
- **Simple instructions**: each instruction performs a single, simple operation.
- **Fixed-length instructions**: simplifies decoding and pipelining.
- **Load/store architecture**: only load and store instructions access memory; arithmetic operates on registers.
- **Large register file**: many general-purpose registers reduce memory traffic.
- **Pipelining-friendly**: uniform instruction format enables deep pipelines.

### Advantages
- Easier to design and verify.
- Higher clock rates and efficient pipelining.
- Typically lower power consumption.
- Compilers can schedule instructions effectively.

### Examples and use cases
- **ARM** and **RISC-V** are the most widely used RISC ISAs today.
- Dominant in smartphones, tablets, embedded systems, and increasingly in laptops and data centers (e.g., Apple Silicon, AWS Graviton).

---

## Section 3 — Complex Instruction Set Computer (CISC) (Presenter 3)

**CISC** is a design philosophy that provides a rich, complex set of instructions, where a single instruction may perform multiple low-level operations (such as memory access and arithmetic combined).

### Core principles
- **Rich instruction set**: many specialized instructions, including multi-step operations.
- **Variable-length instructions**: instructions can vary in size.
- **Memory-to-memory operations**: instructions may operate directly on memory operands.
- **Fewer instructions per task**: aims to reduce the number of instructions needed for a program.
- **Microcode**: complex instructions are often implemented internally via microcode.

### Advantages
- Denser code (smaller programs) due to powerful instructions.
- Easier to write in assembly for early, manually written code.
- Backward compatibility is historically important (notably x86).

### Trade-offs
- More complex hardware and decoding logic.
- Harder to pipeline; modern CISC processors translate complex instructions into simpler internal operations (similar to RISC).
- Generally higher power consumption than RISC.

### Examples and use cases
- **x86 / x86-64** is the dominant CISC ISA, used in virtually all personal computers and servers.
- Emphasis today is on compatibility and raw performance rather than instruction simplicity.

---

## Comparison summary

| Aspect | RISC | CISC |
| --- | --- | --- |
| Instruction complexity | Simple, single-cycle | Complex, multi-step |
| Instruction length | Fixed | Variable |
| Memory access | Load/store only | Many instructions access memory |
| Register count | Large | Smaller (historically) |
| Typical use | Mobile, embedded, efficient | Desktop, server, compatibility |
