#include "scheduler.hpp"
#include <algorithm>
#include <deque>
#include <stdexcept>
#include <vector>

// Round Robin: each process gets at most `quantum` time units per turn,
// preempted back to the end of the ready queue if it doesn't finish.
// Newly-arrived processes are enqueued (in arrival order) before the
// just-preempted process is put back, which is the standard convention and
// avoids letting a process cut in front of jobs that arrived while it ran.
SimResult run_round_robin(std::vector<Process> processes, int quantum) {
    if (quantum <= 0) throw std::invalid_argument("quantum must be positive");

    SimResult result;
    result.algorithm_name = "Round Robin (q=" + std::to_string(quantum) + ")";

    const int n = static_cast<int>(processes.size());

    std::vector<int> by_arrival(n);
    for (int i = 0; i < n; ++i) by_arrival[i] = i;
    std::stable_sort(by_arrival.begin(), by_arrival.end(), [&](int a, int b) {
        return processes[a].arrival_time < processes[b].arrival_time;
    });

    std::deque<int> ready;
    int current_time = 0;
    int arrived_idx = 0;
    int completed = 0;

    auto enqueue_arrivals_up_to = [&](int t) {
        while (arrived_idx < n && processes[by_arrival[arrived_idx]].arrival_time <= t) {
            ready.push_back(by_arrival[arrived_idx]);
            arrived_idx++;
        }
    };

    enqueue_arrivals_up_to(current_time);

    while (completed < n) {
        if (ready.empty()) {
            current_time = processes[by_arrival[arrived_idx]].arrival_time;
            enqueue_arrivals_up_to(current_time);
            continue;
        }

        int idx = ready.front();
        ready.pop_front();
        Process& p = processes[idx];

        if (p.first_run_time == -1) p.first_run_time = current_time;

        int run_length = std::min(quantum, p.remaining_time);
        int end = current_time + run_length;
        result.timeline.push_back({p.pid, current_time, end});

        p.remaining_time -= run_length;
        current_time = end;

        // Arrivals during this slice join the queue before the preempted process.
        enqueue_arrivals_up_to(current_time);

        if (p.remaining_time == 0) {
            p.completion_time = current_time;
            completed++;
        } else {
            ready.push_back(idx);
        }
    }

    result.processes = std::move(processes);
    compute_metrics(result);
    return result;
}
