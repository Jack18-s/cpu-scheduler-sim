#include "scheduler.hpp"
#include <algorithm>

// First-Come First-Served: strictly non-preemptive, ordered by arrival time.
// O(n log n) for the sort, O(n) to simulate.
SimResult run_fcfs(std::vector<Process> processes) {
    SimResult result;
    result.algorithm_name = "FCFS";

    std::stable_sort(processes.begin(), processes.end(),
                      [](const Process& a, const Process& b) {
                          return a.arrival_time < b.arrival_time;
                      });

    int current_time = 0;
    for (auto& p : processes) {
        if (current_time < p.arrival_time) {
            result.timeline.push_back({IDLE_PID, current_time, p.arrival_time});
            current_time = p.arrival_time;
        }
        p.first_run_time = current_time;
        int end = current_time + p.burst_time;
        result.timeline.push_back({p.pid, current_time, end});
        p.remaining_time = 0;
        p.completion_time = end;
        current_time = end;
    }

    result.processes = std::move(processes);
    compute_metrics(result);
    return result;
}
