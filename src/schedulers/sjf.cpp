#include "scheduler.hpp"
#include <algorithm>
#include <queue>
#include <vector>

// Shortest Job First, non-preemptive. Requires burst times to be known up
// front (a classic SJF assumption). Uses a min-heap on burst_time so that,
// among all arrived-but-unrun processes, we always pick the shortest job in
// O(log n) rather than rescanning the ready set every decision point.
SimResult run_sjf(std::vector<Process> processes) {
    SimResult result;
    result.algorithm_name = "SJF (non-preemptive)";

    const int n = static_cast<int>(processes.size());

    // Indices into `processes`, sorted by arrival time so we can pull in
    // newly-arrived jobs as the simulated clock advances.
    std::vector<int> by_arrival(n);
    for (int i = 0; i < n; ++i) by_arrival[i] = i;
    std::stable_sort(by_arrival.begin(), by_arrival.end(), [&](int a, int b) {
        return processes[a].arrival_time < processes[b].arrival_time;
    });

    auto cmp = [&](int a, int b) {
        if (processes[a].burst_time != processes[b].burst_time)
            return processes[a].burst_time > processes[b].burst_time; // min-heap
        return processes[a].arrival_time > processes[b].arrival_time; // tie-break
    };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> ready(cmp);

    int current_time = 0;
    int arrived_idx = 0;
    int completed = 0;

    while (completed < n) {
        // Pull in every process that has arrived by now.
        while (arrived_idx < n && processes[by_arrival[arrived_idx]].arrival_time <= current_time) {
            ready.push(by_arrival[arrived_idx]);
            arrived_idx++;
        }

        if (ready.empty()) {
            // Nothing to run yet; jump straight to the next arrival.
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
