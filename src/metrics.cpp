#include "scheduler.hpp"
#include <algorithm>

void compute_metrics(SimResult& result) {
    const auto& processes = result.processes;
    const auto& timeline = result.timeline;

    if (processes.empty()) return;

    double total_waiting = 0.0, total_turnaround = 0.0, total_response = 0.0;
    int makespan = 0;

    for (const auto& p : processes) {
        total_waiting += p.waiting_time();
        total_turnaround += p.turnaround_time();
        total_response += p.response_time();
        makespan = std::max(makespan, p.completion_time);
    }

    const int n = static_cast<int>(processes.size());
    result.metrics.avg_waiting_time = total_waiting / n;
    result.metrics.avg_turnaround_time = total_turnaround / n;
    result.metrics.avg_response_time = total_response / n;
    result.metrics.makespan = makespan;
    result.metrics.throughput = makespan > 0 ? static_cast<double>(n) / makespan : 0.0;

    // Count switches to a *different* pid across consecutive scheduled slices.
    // Idle gaps (pid == -1) are treated like any other "process" for this
    // purpose, since resuming from idle also costs a dispatch.
    int switches = 0;
    for (size_t i = 1; i < timeline.size(); ++i) {
        if (timeline[i].pid != timeline[i - 1].pid) switches++;
    }
    result.metrics.context_switches = switches;
}
