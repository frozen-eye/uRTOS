# uRTOS

A small educational RTOS with **preemptive scheduling** for bare-metal QEMU `virt` machines.

Supported targets:

| Port | Architecture | QEMU machine | Toolchain |
|------|--------------|--------------|-----------|
| `aarch64` (default) | ARM 64-bit | `qemu-system-aarch64 -M virt` | `aarch64-linux-gnu-gcc` |
| `armhf` | ARM 32-bit hard-float | `qemu-system-arm -M virt` | `arm-linux-gnueabihf-gcc` |
| `riscv64` | RISC-V 64-bit | `qemu-system-riscv64 -M virt` | `riscv64-linux-gnu-gcc` |
| `riscv32` | RISC-V 32-bit | `qemu-system-riscv32 -M virt` | `riscv32-linux-gnu-gcc` |

## Repository

https://github.com/uRTOS/uRTOS

## License

MIT — see [LICENSE](LICENSE).

## Build

Select a port with the `PORT` variable:

```bash
make clean
make                  # aarch64 (default)
make PORT=armhf
make PORT=riscv64
make PORT=riscv32
```

Install the matching cross toolchain on Debian/Ubuntu:

```bash
sudo apt install gcc-aarch64-linux-gnu          # aarch64
sudo apt install gcc-arm-linux-gnueabihf        # armhf
sudo apt install gcc-riscv64-linux-gnu          # riscv64
sudo apt install gcc-riscv32-linux-gnu          # riscv32
sudo apt install qemu-system-misc               # qemu-system-riscv32
```

## Run under QEMU

```bash
make run                  # uses current PORT
make PORT=aarch64 run
make PORT=armhf run
make PORT=riscv64 run
make PORT=riscv32 run
```

Convenience targets that rebuild and run:

```bash
make run-aarch64
make run-armhf
make run-riscv64
make run-riscv32
```

### Manual QEMU commands

**AArch64**

```bash
qemu-system-aarch64 \
  -M virt,gic-version=2 \
  -cpu cortex-a53 \
  -nographic \
  -serial mon:stdio \
  -kernel build/kernel.elf
```

**ARM 32-bit (armhf)**

```bash
qemu-system-arm \
  -M virt,highmem=off \
  -cpu cortex-a15 \
  -nographic \
  -serial mon:stdio \
  -kernel build/kernel.elf
```

**RISC-V 64-bit**

```bash
qemu-system-riscv64 \
  -M virt \
  -cpu rv64 \
  -nographic \
  -serial mon:stdio \
  -kernel build/kernel.elf
```

**RISC-V 32-bit**

```bash
qemu-system-riscv32 \
  -M virt \
  -cpu rv32 \
  -nographic \
  -serial mon:stdio \
  -kernel build/kernel.elf
```

## What works today (v0.3)

- Bare-metal boot on all three QEMU `virt` ports
- **No MMU**, no Linux, no U-Boot
- PL011 UART (ARM) or NS16550 UART (RISC-V)
- Static task stacks (no heap)
- Round-robin preemptive scheduler
- Cooperative `os_yield()`
- `os_task_delay()` using the kernel tick (`OS_TICK_HZ` = 1000)
- Timer-driven preemption:
  - **aarch64 / armhf** — ARM Generic Timer + GICv2 (IRQ 30)
  - **riscv64 / riscv32** — CLINT machine timer (`mtime` / `mtimecmp`)
- Exception/trap vectors with panic on unexpected faults

## How timer preemption works

1. Boot code installs the CPU exception/trap vector table and initializes the platform timer and interrupt controller.
2. When the tick timer fires, the **IRQ/trap handler** saves the full register frame of the interrupted task, switches to a dedicated IRQ stack, and calls `os_irq_handler()`.
3. The handler calls `os_tick_handler()` (increments tick, wakes delayed tasks, runs the scheduler), reprograms the timer, and completes interrupt processing.
4. If the scheduler picks a different task, `os_current_task` is updated; handler exit restores the **new** task's frame and returns into it.
5. A CPU-bound task that never calls `os_yield()` is still preempted every tick.

## Demo output

The basic example runs three tasks:

- **Task A** — prints every 1000 ms
- **Task B** — prints every 500 ms
- **Task C** — busy-spins without yielding (proves preemption)

Expected pattern (banner string varies by port):

```text
uRTOS/qemu-aarch64 boot
[kernel] scheduler started
[tick] timer frequency: XXXXXXXX Hz
[0 ms] task A: alive
[0 ms] task B: alive
[500 ms] task B: alive
[1000 ms] task A: alive
[1000 ms] task B: alive
...
```

## Project layout

```text
Makefile
LICENSE
include/                    Public headers and os_config.h
kernel/                     Scheduler, panic
boards/common/              Shared boot entry
boards/qemu-virt-aarch64/   AArch64 linker script
boards/qemu-virt-armhf/     ARM32 linker script
boards/qemu-virt-riscv64/   RISC-V 64-bit linker script
boards/qemu-virt-riscv32/   RISC-V 32-bit linker script
port/aarch64-qemu-virt/     AArch64 boot, vectors, GIC, timer
port/armhf-qemu-virt/       ARM32 boot, vectors, GIC, timer
port/riscv64-qemu-virt/     RISC-V 64-bit boot, traps, CLINT timer
port/riscv32-qemu-virt/     RISC-V 32-bit boot, traps, CLINT timer
examples/basic/             Preemption demo
```

## Current limitations

- No MMU, MPU, or user/supervisor separation
- No mutexes, queues, or dynamic allocation
- Round-robin only (no priorities)
- Context switch from IRQ/trap context directly (no deferred scheduler call)
- **aarch64 / armhf**: GICv2 only (`gic-version=2` on AArch64 QEMU)
- **riscv64 / riscv32**: M-mode only (no S-mode / SBI)
- Single-core only
- Minimal fault diagnostics (panic string only)
- `os_task_delay()` resolution is one tick (1 ms at 1000 Hz)

## TODO

- Symmetric multi-processing (SMP) across cores
- GICv3 support for newer QEMU defaults
- RISC-V S-mode port with SBI timer
- Deferred scheduler call from IRQ (reduce interrupt latency)
- Task priorities and synchronization primitives
- Better fault reporting (ESR / mcause / mtval dump)
