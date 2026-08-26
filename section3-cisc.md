# Section 3 — Complex Instruction Set Computer (CISC)

## 3.1 Core Principles
![CISC microcode translation diagram](cisc-diagram.png)

CISC offers a rich instruction set where one instruction may do several low-level steps at once.

- **Rich instructions**: many specialized, powerful operations.
- **Variable length**: instructions differ in size.
- **Memory operands**: instructions can operate directly on memory.
- **Fewer instructions per task**: aims for compact programs.
- **Microcode**: complex instructions are internally broken into simpler steps.

## 3.2 Advantages
- Denser machine code from powerful instructions.
- Strong backward compatibility (notably x86).
- Historically easier hand-written assembly.

## 3.3 Trade-offs and Use Cases
- More complex decoding; modern CISC translates to RISC-like internals.
- Higher power draw than RISC.
- **x86 / x86-64** dominates desktops and servers where compatibility and performance matter most.
