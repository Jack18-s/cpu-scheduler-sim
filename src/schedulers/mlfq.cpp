#include "scheduler.hpp"
#include <algorithm>
#include <deque>
#include <stdexcept>
#include <vector>

// Multi-Level Feedback Queue: quanta.size() priority levels (0 = highest
// priority, shortest quantum). New arrivals start at level 0. A process
// that exhausts its quantum without finishing is demoted one level (capped
// at the lowest level) -- this is what makes CPU-bound jobs sink over time
// while short/interactive jobs stay near the top. A periodic priority
// boost moves every waiting process back to level 0 to prevent long jobs
// from starving forever once they've sunk to the bottom.
//
// Simplification: this models pure CPU-bound processes (no I/O bursts), so
// a process only ever leaves a queue by finishing or by exhausting its
// quantum -- there's no separate "returned from I/O" promotion path here.
SimResult run_mlfq(std::vector<Process> processes, std::vector<int> quanta, int boost_interval) {
    if (quanta.empty()) throw std::invalid_argument("MLFQ needs at least one queue level");
    for (int q : quanta) {
        if (q <= 0) throw std::invalid_argument("all MLFQ quanta must be positive");
    }

    SimResult result;
    result.algorithm_name = "MLFQ (" + std::to_string(quanta.size()) + " levels)";

    const int n = static_cast<int>(processes.size());
    const int levels = static_cast<int>(quanta.size());

    std::vector<int> by_arrival(n);
    for (int i = 0; i < n; ++i) by_arrival[i] = i;
    std::stable_sort(by_arrival.begin(), by_arrival.end(), [&](int a, int b) {
        return processes[a].arrival_time < processes[b].arrival_time;
    });

    std::vector<std::deque<int>> queues(levels);
    int current_time = 0;
    int arrived_idx = 0;
    int completed = 0;
    int last_boost = 0;

    auto enqueue_arrivals_up_to = [&](int t) {
        while (arrived_idx < n && processes[by_arrival[arrived_idx]].arrival_time <= t) {
            queues[0].push_back(by_arrival[arrived_idx]); // new arrivals start at the top
            arrived_idx++;
        }
    };

    enqueue_arrivals_up_to(current_time);

    while (completed < n) {
        // Priority boost: pull every still-waiting process back to level 0.
        if (boost_interval > 0 && current_time - last_boost >= boost_interval) {
            for (int lvl = 1; lvl < levels; ++lvl) {
                while (!queues[lvl].empty()) {
                    queues[0].push_back(queues[lvl].front());
                    queues[lvl].pop_front();
                }
            }
            last_boost = current_time;
        }

        int lvl = -1;
        for (int i = 0; i < levels; ++i) {
            if (!queues[i].empty()) { lvl = i; break; }
        }

        if (lvl == -1) {
            current_time = processes[by_arrival[arrived_idx]].arrival_time;
            enqueue_arrivals_up_to(current_time);
            continue;
        }

        int idx = queues[lvl].front();
        queues[lvl].pop_front();
        Process& p = processes[idx];

        if (p.first_run_time == -1) p.first_run_time = current_time;

        int quantum = quanta[lvl];
        int run_length = std::min(quantum, p.remaining_time);
        int end = current_time + run_length;
        result.timeline.push_back({p.pid, current_time, end});

        p.remaining_time -= run_length;
        current_time = end;

        enqueue_arrivals_up_to(current_time);

        if (p.remaining_time == 0) {
            p.completion_time = current_time;
            completed++;
        } else {
            // Used the whole quantum without finishing -> demote (capped at bottom level).
            int demoted_lvl = std::min(lvl + 1, levels - 1);
            queues[demoted_lvl].push_back(idx);
        }
    }

    result.processes = std::move(processes);
    compute_metrics(result);
    return result;
}
