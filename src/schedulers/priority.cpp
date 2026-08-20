#include "scheduler.hpp"
#include <algorithm>
#include <queue>
#include <vector>

// Non-preemptive Priority Scheduling: among arrived-but-unrun processes,
// always run the one with the numerically lowest `priority` value (lower
// number = higher priority), breaking ties by arrival time. Static
// priorities only — no aging, so starvation of low-priority jobs is
// possible (documented as a known limitation; MLFQ is the fix for this).
SimResult run_priority(std::vector<Process> processes) {
    SimResult result;
    result.algorithm_name = "Priority (non-preemptive)";

    const int n = static_cast<int>(processes.size());

    std::vector<int> by_arrival(n);
    for (int i = 0; i < n; ++i) by_arrival[i] = i;
    std::stable_sort(by_arrival.begin(), by_arrival.end(), [&](int a, int b) {
        return processes[a].arrival_time < processes[b].arrival_time;
    });

    auto cmp = [&](int a, int b) {
        if (processes[a].priority != processes[b].priority)
            return processes[a].priority > processes[b].priority; // min-heap on priority value
        return processes[a].arrival_time > processes[b].arrival_time;
    };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> ready(cmp);

    int current_time = 0;
    int arrived_idx = 0;
    int completed = 0;

    while (completed < n) {
        while (arrived_idx < n && processes[by_arrival[arrived_idx]].arrival_time <= current_time) {
            ready.push(by_arrival[arrived_idx]);
            arrived_idx++;
        }

        if (ready.empty()) {
            int next_arrival = processes[by_arrival[arrived_idx]].arrival_time;
            result.timeline.push_back({IDLE_PID, current_time, next_arrival});
            current_time = next_arrival;
            continue;
        }

        int idx = ready.top();
        ready.pop();
        Process& p = processes[idx];

        p.first_run_time = current_time;
        int end = current_time + p.burst_time;
        result.timeline.push_back({p.pid, current_time, end});
        p.remaining_time = 0;
        p.completion_time = end;
        current_time = end;
        completed++;
    }

    result.processes = std::move(processes);
    compute_metrics(result);
    return result;
}
