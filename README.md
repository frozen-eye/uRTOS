# uRTOS

A small educational RTOS with **preemptive, priority-aware scheduling** and **SMP** (on ARM) for bare-metal QEMU `virt` machines.

Supported targets:

| Port | Architecture | QEMU machine | SMP |
|------|--------------|--------------|-----|
| `aarch64` (default) | ARM 64-bit | `qemu-system-aarch64 -M virt -smp 2` | yes (2 CPUs) |
| `armhf` | ARM 32-bit hard-float | `qemu-system-arm -M virt -smp 2` | yes (2 CPUs) |
| `riscv64` | RISC-V 64-bit | `qemu-system-riscv64 -M virt` | no |
| `riscv32` | RISC-V 32-bit | `qemu-system-riscv32 -M virt` | no |

Toolchains: `aarch64-linux-gnu-gcc`, `arm-linux-gnueabihf-gcc`, `riscv64-linux-gnu-gcc`, `riscv32-linux-gnu-gcc`.

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
  -smp 2 \
  -nographic \
  -serial mon:stdio \
  -kernel build/kernel.elf
```

**ARM 32-bit (armhf)**

```bash
qemu-system-arm \
  -M virt,highmem=off \
  -cpu cortex-a15 \
  -smp 2 \
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

## What works today (v0.6)

- Bare-metal boot on all four QEMU `virt` ports
- **No MMU**, no Linux, no U-Boot
- PL011 UART (ARM) or NS16550 UART (RISC-V)
- Dynamic task stacks and TCBs via bump heap (`os_mem_alloc`)
- Round-robin among **equal-priority** ready tasks; higher numeric priority preempts lower
- Cooperative `os_yield()`
- `os_task_delay()` using the kernel tick (`OS_TICK_HZ` = 1000)
- **SMP on aarch64 and armhf** (`OS_CPU_COUNT=2`, QEMU `-smp 2`):
  - Per-CPU state via `os_cpus[]` and `TPIDR_EL1` / `TPIDRPRW`
  - Spinlock-protected global scheduler with **work-stealing**
  - GIC SGI IPI for cross-CPU reschedule
  - Timer tick on CPU0 only; secondaries boot via PSCI `CPU_ON`
  - `os_task_bind()` to pin a task to a specific core
  - Mutexes and counting semaphores (`os_mutex_*`, `os_sem_*`)
  - `os_cpu_id()` in task output
- Timer-driven preemption:
  - **aarch64** — ARM Generic Timer + GICv2 (IRQ 30)
  - **armhf** — ARM Generic Timer + GIC (IRQ 27)
  - **riscv64 / riscv32** — CLINT machine timer (`mtime` / `mtimecmp`)
- Exception/trap vectors with panic on unexpected faults

## How timer preemption works

1. Boot code installs the CPU exception/trap vector table and initializes the platform timer and interrupt controller.
2. When the tick timer fires, the **IRQ/trap handler** saves the full register frame of the interrupted task, switches to a dedicated IRQ stack, and calls `os_irq_handler()`.
3. The handler calls `os_tick_handler()` (increments tick, wakes delayed tasks, runs the scheduler), reprograms the timer, and completes interrupt processing.
4. If the scheduler picks a different task, the per-CPU `current` pointer is updated; handler exit restores the **new** task's frame and returns into it.
5. A CPU-bound task that never calls `os_yield()` is still preempted every tick.

## Task priorities

Priorities range from `OS_TASK_PRIO_MIN` (0) to `OS_TASK_PRIO_MAX` (7); **higher number = higher priority**. Pass the priority to `os_task_create()`, or change it later with `os_task_set_priority()`. Among ready tasks with the same priority the scheduler still round-robins.

```c
os_task_create("sensor", sensor_task, NULL, NULL, OS_TASK_PRIO_HIGH);
os_task_set_priority(task, OS_TASK_PRIO_REALTIME);
```

## Mutexes and semaphores

Blocking synchronization with per-object wait queues. Tasks block in `OS_TASK_BLOCKED` and are woken on `unlock` / `post`. Mutexes support **direct handoff** to the next waiter; semaphores are counting.

```c
os_mutex_t mu;
os_sem_t   sem;

os_mutex_init(&mu);
os_sem_init(&sem, 0);

os_mutex_lock(&mu);
os_mutex_unlock(&mu);

if (os_mutex_trylock(&mu) == 0) { /* acquired */ }

os_sem_wait(&sem);
os_sem_post(&sem);
```

Return value for `try` variants: `0` on success, `-1` if the call would block. Unlocking a mutex not owned by the current task panics.

## Work-stealing (SMP)

When a CPU finds no locally runnable task, the scheduler performs a steal pass: it picks a `READY` task currently attributed to another CPU (`run_cpu`), respecting `aff_cpu` affinity. The previous owner is cleared and notified via IPI. This lets idle cores pull delayed tasks while a CPU-bound task spins elsewhere.

## Demo output

The basic example runs four tasks:

- **Task A** — normal priority, prints every 1000 ms
- **Task B** — high priority, prints every 500 ms
- **prod / cons** — semaphore + mutex; producer posts every 400 ms, consumer increments a shared counter

Expected pattern (banner string varies by port):

```text
uRTOS/qemu-aarch64 boot
[kernel] scheduler started (SMP)
[tick] timer frequency: XXXXXXXX Hz
[0 ms cpu0] task B: alive
[500 ms cpu0] task B: alive
[1000 ms cpu0] task B: alive
[1000 ms cpu1] task A: alive
[400 ms cpu0] count=1
[800 ms cpu1] count=2
...
```

Task B (high priority) prints about twice as often as task A (normal). Exact CPU assignment varies with SMP scheduling and work-stealing.

## Project layout

```text
Makefile
LICENSE
include/                    Public headers and os_config.h
kernel/                     Scheduler, heap, sync, SMP, spinlock, panic
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
- No priority inheritance; mutex/semaphore wait queues are FIFO
- Bump-heap allocator only (`os_mem_alloc` / `os_mem_free`); no `free()` coalescing
- Fixed-priority preemptive scheduler
- Context switch from IRQ/trap context directly (no deferred scheduler call)
- **SMP** on **aarch64 / armhf** only (`OS_CPU_COUNT=2`); RISC-V ports are unicore
- **aarch64 / armhf**: GICv2 only (`gic-version=2` on AArch64 QEMU)
- **riscv64 / riscv32**: M-mode only (no S-mode / SBI)
- Minimal fault diagnostics (panic string only)
- `os_task_delay()` resolution is one tick (1 ms at 1000 Hz)

## TODO

- GICv3 support for newer QEMU defaults
- RISC-V S-mode port with SBI timer
- SMP on RISC-V (`-smp` + IPI)
- Deferred scheduler call from IRQ (reduce interrupt latency)
- Message queues and event groups
- Better fault reporting (ESR / mcause / mtval dump)
