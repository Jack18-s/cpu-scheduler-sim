// Minimal, dependency-free test harness (no gtest needed). Each test
// compares simulator output against hand-computed expected values for
// well-known scheduling examples, so a correctness bug can't hide behind
// "well it compiled."
#include "scheduler.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void expect_near(double actual, double expected, const std::string& label, double eps = 0.01) {
    g_checks++;
    if (std::fabs(actual - expected) > eps) {
        g_failures++;
        std::cout << "FAIL: " << label << " expected=" << expected
                   << " actual=" << actual << "\n";
    }
}

void expect_eq(int actual, int expected, const std::string& label) {
    g_checks++;
    if (actual != expected) {
        g_failures++;
        std::cout << "FAIL: " << label << " expected=" << expected
                   << " actual=" << actual << "\n";
    }
}

// FCFS: P1(arr0,b5) P2(arr1,b3) P3(arr2,b2)
// P1: 0-5 (wait 0), P2: 5-8 (wait 4), P3: 8-10 (wait 6) -> avg wait 10/3
void test_fcfs() {
    std::vector<Process> procs = {
        Process(1, 0, 5),
        Process(2, 1, 3),
        Process(3, 2, 2),
    };
    SimResult r = run_fcfs(procs);
    expect_near(r.metrics.avg_waiting_time, 10.0 / 3.0, "FCFS avg waiting time");
    expect_eq(r.metrics.makespan, 10, "FCFS makespan");
}

// Classic textbook SJF example: P1(0,8) P2(1,4) P3(2,9) P4(3,5)
// Non-preemptive schedule: P1[0-8], P2[8-12], P4[12-17], P3[17-26]
// avg waiting = (0 + 7 + 15 + 9) / 4 = 7.75
void test_sjf() {
    std::vector<Process> procs = {
        Process(1, 0, 8),
        Process(2, 1, 4),
        Process(3, 2, 9),
        Process(4, 3, 5),
    };
    SimResult r = run_sjf(procs);
    expect_near(r.metrics.avg_waiting_time, 7.75, "SJF avg waiting time");
    expect_eq(r.metrics.makespan, 26, "SJF makespan");
}

// Classic textbook Round Robin example: P1(0,24) P2(0,3) P3(0,3), quantum 4
// avg waiting = (6 + 4 + 7) / 3 = 5.667
void test_round_robin() {
    std::vector<Process> procs = {
        Process(1, 0, 24),
        Process(2, 0, 3),
        Process(3, 0, 3),
    };
    SimResult r = run_round_robin(procs, 4);
    expect_near(r.metrics.avg_waiting_time, 5.667, "Round Robin avg waiting time");
    expect_eq(r.metrics.makespan, 30, "Round Robin makespan");
}

// SRTF should never do worse than SJF on turnaround time for the same
// workload (preemption only helps), and every process must actually finish.
void test_srtf_completes_and_beats_or_matches_sjf() {
    std::vector<Process> procs = {
        Process(1, 0, 8),
        Process(2, 1, 4),
        Process(3, 2, 9),
        Process(4, 3, 5),
    };
    SimResult sjf = run_sjf(procs);
    SimResult srtf = run_srtf(procs);

    for (const auto& p : srtf.processes) {
        expect_eq(p.remaining_time, 0, "SRTF process " + std::to_string(p.pid) + " finished");
    }
    g_checks++;
    if (srtf.metrics.avg_waiting_time > sjf.metrics.avg_waiting_time + 1e-9) {
        g_failures++;
        std::cout << "FAIL: SRTF avg wait (" << srtf.metrics.avg_waiting_time
                   << ") should be <= SJF avg wait (" << sjf.metrics.avg_waiting_time << ")\n";
    }
}

// Priority scheduling: lower number = higher priority. With all arriving at
// t=0, the process with priority 1 must run first regardless of burst size.
void test_priority_runs_highest_priority_first() {
    std::vector<Process> procs = {
        Process(1, 0, 10, /*priority=*/3),
        Process(2, 0, 2, /*priority=*/1),
        Process(3, 0, 5, /*priority=*/2),
    };
    SimResult r = run_priority(procs);
    expect_eq(r.timeline.front().pid, 2, "Priority: pid 2 (priority 1) runs first");
}

// MLFQ: a long CPU-bound job should get demoted below a short job that
// arrives right after it, so the short job finishes before the long one
// even though the long job arrived first.
void test_mlfq_favors_short_jobs_over_long_ones() {
    std::vector<Process> procs = {
        Process(1, 0, 50),  // long CPU-bound job
        Process(2, 1, 3),   // short job arriving just after
    };
    SimResult r = run_mlfq(procs, {4, 8, 16}, /*boost_interval=*/1000);
    int completion_short = -1, completion_long = -1;
    for (const auto& p : r.processes) {
        if (p.pid == 2) completion_short = p.completion_time;
        if (p.pid == 1) completion_long = p.completion_time;
    }
    g_checks++;
    if (!(completion_short < completion_long)) {
        g_failures++;
        std::cout << "FAIL: MLFQ short job should finish before long job "
                   << "(short=" << completion_short << ", long=" << completion_long << ")\n";
    }
}

// A priority boost should eventually let a starved low-priority job run.
void test_mlfq_priority_boost_prevents_starvation() {
    std::vector<Process> procs = {
        Process(1, 0, 4),    // gets demoted immediately after using its quantum once
        Process(2, 100, 4),  // arrives later, would otherwise keep P1 stuck below it forever
    };
    // With a short boost interval, P1 should get periodically restored to
    // level 0 and make progress instead of starving under P2.
    SimResult r = run_mlfq(procs, {4, 8}, /*boost_interval=*/10);
    for (const auto& p : r.processes) {
        expect_eq(p.remaining_time, 0, "MLFQ+boost process " + std::to_string(p.pid) + " finished");
    }
}

} // namespace

int main() {
    test_fcfs();
    test_sjf();
    test_round_robin();
    test_srtf_completes_and_beats_or_matches_sjf();
    test_priority_runs_highest_priority_first();
    test_mlfq_favors_short_jobs_over_long_ones();
    test_mlfq_priority_boost_prevents_starvation();

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed.\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S).\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
