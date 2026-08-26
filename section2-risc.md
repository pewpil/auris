# Section 2 — Reduced Instruction Set Computer (RISC)

## 2.1 Core Principles
![RISC pipeline diagram](risc-diagram.png)

RISC uses a small set of simple instructions, each typically completed in one clock cycle.

- **Simple instructions**: one operation per instruction.
- **Fixed length**: uniform encoding simplifies decoding.
- **Load/store**: only load/store touch memory; math uses registers.
- **Large register file**: many registers reduce memory traffic.
- **Pipelining-friendly**: regular format enables deep pipelines.

## 2.2 Advantages
- Simpler, easier-to-verify hardware design.
- Higher clock speeds and efficient pipelines.
- Lower power consumption.
- Compilers can schedule and optimize effectively.

## 2.3 Examples and Use Cases
- **ARM** and **RISC-V** are the leading RISC ISAs.
- Dominant in smartphones, tablets, embedded devices, and growing in laptops and cloud (Apple Silicon, AWS Graviton).
