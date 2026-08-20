# CPU Scheduler Simulator

A discrete-event CPU scheduling simulator implemented in modern C++ (C++17).
It implements six classic scheduling algorithms, runs them against the same
workload, and reports the standard performance metrics used to compare
scheduling policies (average waiting time, turnaround time, response time,
throughput, and context-switch count).

## Algorithms implemented

| Algorithm | Preemptive? | Data structure used | Notes |
|---|---|---|---|
| First-Come First-Served (FCFS) | No | sorted array | Baseline; fair by arrival order only |
| Shortest Job First (SJF) | No | min-heap on burst time | Optimal average waiting time *if* all jobs are available at once |
| Shortest Remaining Time First (SRTF) | Yes | min-heap on remaining time | Preemptive SJF; re-evaluated every tick |
| Round Robin (RR) | Yes | FIFO queue | Time-quantum based; fairness guarantee |
| Priority Scheduling | No | min-heap on priority | Static priority, no aging → can starve |
| Multi-Level Feedback Queue (MLFQ) | Yes | `N` FIFO queues + demotion/boost | Approximates SJF without knowing burst times in advance; includes starvation-prevention priority boost |

## Design

- **`Process`** (`include/process.hpp`) holds arrival time, burst time,
  priority, and the fields filled in during simulation (`first_run_time`,
  `completion_time`), plus derived accessors for turnaround/waiting/response
  time.
- Each scheduler is a standalone function (`run_fcfs`, `run_sjf`, ...) that
  takes a *copy* of the workload and returns a `SimResult`: the full
  dispatch timeline (`ScheduleSlice` list, usable to render a Gantt chart)
  and the final per-process stats. This keeps algorithms independently
  testable and lets `main.cpp` run all six against the same input without
  them interfering with each other.
- Non-preemptive algorithms (FCFS, SJF, Priority) are simulated
  event-by-event: the clock jumps directly from one dispatch decision to
  the next, so runtime is `O(n log n)` regardless of burst-time magnitude.
- Preemptive algorithms (SRTF, RR, MLFQ) are simulated in discrete ticks
  while the CPU is busy (since a shorter job can arrive and preempt at any
  moment), but idle stretches still jump straight to the next arrival
  rather than ticking through empty time.
- `compute_metrics()` (`src/metrics.cpp`) is shared by every algorithm so
  the definitions of waiting/turnaround/response time and context-switch
  counting can't drift between schedulers.
- MLFQ models pure CPU-bound processes (no I/O bursts): a process leaves a
  queue either by finishing or by exhausting its quantum (demotion). A
  configurable periodic priority boost resets every waiting process back to
  the top queue to prevent long jobs from starving once they've sunk to the
  bottom level — this is called out explicitly as a simplification in
  `src/schedulers/mlfq.cpp`.

## Building and running

```bash
make            # builds ./scheduler_sim
make test       # builds and runs ./run_tests (correctness checks)
make run        # runs scheduler_sim against data/workload1.csv with a Gantt chart
```

Manual usage:

```bash
./scheduler_sim <workload.csv> [--quantum N] [--mlfq-quanta a,b,c] [--gantt]
```

`workload.csv` format (header optional):

```
pid,arrival_time,burst_time,priority
1,0,8,2
2,1,4,1
...
```

## Sample output

```
$ ./scheduler_sim data/workload1.csv
Algorithm                       Avg Wait  Avg Turnaround  Avg Response  Throughput  CtxSw  Makespan
----------------------------------------------------------------------------------------------------
FCFS                                11.40           17.00         11.40        0.18      4        28
SJF (non-preemptive)                 8.20           13.80          8.20        0.18      4        28
SRTF (preemptive)                    6.60           12.20          4.40        0.18      5        28
Round Robin (q=4)                   13.00           18.60          6.00        0.18      8        28
Priority (non-preemptive)            8.60           14.20          8.60        0.18      4        28
MLFQ (3 levels)                     13.00           18.60          6.00        0.18      7        28
```

SRTF gives the best turnaround/waiting time (as expected — it's provably
optimal for average waiting time among preemptive policies with known burst
times), while Round Robin and MLFQ trade some average-case efficiency for
much better *response* time, which matters more for interactive workloads.

## Testing

`tests/test_schedulers.cpp` is a small dependency-free test harness that
checks simulator output against hand-computed expected values for
well-known textbook examples (e.g. the standard SJF and Round Robin
worked examples), plus structural invariants (every process finishes,
SRTF never does worse than SJF, priority scheduling picks the highest
priority job first, MLFQ favors short jobs and its priority boost prevents
starvation).

```bash
make test
```

## Possible extensions

- Model I/O bursts per process (alternating CPU/IO phases) for a more
  realistic MLFQ promotion path.
- Preemptive priority scheduling with aging.
- Multi-core / SMP scheduling with load balancing across per-core ready
  queues.
- CSV metrics export + plotting script for visual algorithm comparison.
