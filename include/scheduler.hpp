#pragma once
#include "process.hpp"
#include <vector>

// Sentinel pid used in ScheduleSlice to represent the CPU sitting idle
// (no process has arrived yet, or the ready queue is momentarily empty).
constexpr int IDLE_PID = -1;

struct Metrics {
    double avg_waiting_time = 0.0;
    double avg_turnaround_time = 0.0;
    double avg_response_time = 0.0;
    double throughput = 0.0;      // completed processes per unit time
    int context_switches = 0;     // number of times the CPU switched to a different pid
    int makespan = 0;             // total time until the last process finishes
};

struct SimResult {
    std::string algorithm_name;
    std::vector<ScheduleSlice> timeline;
    std::vector<Process> processes; // final state, with completion/first_run filled in
    Metrics metrics;
};

// Computes avg waiting/turnaround/response time, throughput, context switches,
// and makespan from a finished SimResult's timeline + processes. Called
// internally by every scheduler at the end of its run.
void compute_metrics(SimResult& result);

// ---- Scheduling algorithms ----
// Each takes a copy of the input processes (so callers can reuse the same
// workload across algorithms) and returns a fully populated SimResult.

SimResult run_fcfs(std::vector<Process> processes);

SimResult run_sjf(std::vector<Process> processes); // non-preemptive

SimResult run_srtf(std::vector<Process> processes); // preemptive shortest-remaining-time

SimResult run_round_robin(std::vector<Process> processes, int quantum);

SimResult run_priority(std::vector<Process> processes); // non-preemptive, lower value = higher priority

// quanta[i] is the time quantum for queue level i (0 = highest priority).
// boost_interval > 0 periodically moves every process back to queue 0 to
// prevent starvation of long-running jobs.
SimResult run_mlfq(std::vector<Process> processes,
                    std::vector<int> quanta = {4, 8, 16},
                    int boost_interval = 50);
