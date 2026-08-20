#include "scheduler.hpp"
#include <algorithm>
#include <queue>
#include <vector>

// Shortest Remaining Time First: the preemptive sibling of SJF. Whenever a
// newly-arrived process has a shorter remaining burst than whoever is
// currently running, it preempts immediately.
//
// Implemented as a 1-tick-at-a-time simulation (rather than a fully
// event-driven jump-to-next-event scheduler) because remaining_time changes
// every tick for the running process, which is what can trigger a
// preemption at any moment. Idle stretches still jump straight to the next
// arrival rather than ticking through empty time, so the O(makespan) factor
// only applies while the CPU is actually busy.
SimResult run_srtf(std::vector<Process> processes) {
    SimResult result;
    result.algorithm_name = "SRTF (preemptive)";

    const int n = static_cast<int>(processes.size());

    std::vector<int> by_arrival(n);
    for (int i = 0; i < n; ++i) by_arrival[i] = i;
    std::stable_sort(by_arrival.begin(), by_arrival.end(), [&](int a, int b) {
        return processes[a].arrival_time < processes[b].arrival_time;
    });

    auto cmp = [&](int a, int b) {
        if (processes[a].remaining_time != processes[b].remaining_time)
            return processes[a].remaining_time > processes[b].remaining_time; // min-heap
        return processes[a].arrival_time > processes[b].arrival_time;
    };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> ready(cmp);

    int current_time = 0;
    int arrived_idx = 0;
    int completed = 0;

    bool has_open_slice = false;
    int slice_pid = IDLE_PID;
    int slice_start = 0;

    auto close_slice_if_open = [&](int at_time) {
        if (has_open_slice && slice_start < at_time) {
            result.timeline.push_back({slice_pid, slice_start, at_time});
        }
    };

    while (completed < n) {
        while (arrived_idx < n && processes[by_arrival[arrived_idx]].arrival_time <= current_time) {
            ready.push(by_arrival[arrived_idx]);
            arrived_idx++;
        }

        int this_pid = IDLE_PID;
        int idx = -1;
        if (!ready.empty()) {
            idx = ready.top();
            this_pid = processes[idx].pid;
        }

        if (!has_open_slice || this_pid != slice_pid) {
            close_slice_if_open(current_time);
            slice_pid = this_pid;
            slice_start = current_time;
            has_open_slice = true;
        }

        if (idx == -1) {
            // CPU idle: skip straight to the next arrival instead of ticking.
            current_time = processes[by_arrival[arrived_idx]].arrival_time;
            continue;
        }

        ready.pop();
        Process& p = processes[idx];
        if (p.first_run_time == -1) p.first_run_time = current_time;

        p.remaining_time -= 1;
        current_time += 1;

        if (p.remaining_time == 0) {
            p.completion_time = current_time;
            completed++;
        } else {
            ready.push(idx);
        }
    }

    close_slice_if_open(current_time);

    result.processes = std::move(processes);
    compute_metrics(result);
    return result;
}
