#include "process.hpp"
#include "scheduler.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void print_metrics_table(const std::vector<SimResult>& results) {
    std::cout << std::left
               << std::setw(28) << "Algorithm"
               << std::right
               << std::setw(12) << "Avg Wait"
               << std::setw(14) << "Avg Turnaround"
               << std::setw(14) << "Avg Response"
               << std::setw(12) << "Throughput"
               << std::setw(10) << "CtxSw"
               << std::setw(10) << "Makespan"
               << "\n";
    std::cout << std::string(100, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(28) << r.algorithm_name
                   << std::right << std::fixed << std::setprecision(2)
                   << std::setw(12) << r.metrics.avg_waiting_time
                   << std::setw(14) << r.metrics.avg_turnaround_time
                   << std::setw(14) << r.metrics.avg_response_time
                   << std::setw(12) << r.metrics.throughput
                   << std::setw(10) << r.metrics.context_switches
                   << std::setw(10) << r.metrics.makespan
                   << "\n";
    }
}

void print_gantt(const SimResult& r) {
    std::cout << "\n" << r.algorithm_name << " Gantt chart:\n";
    for (const auto& slice : r.timeline) {
        std::string label = slice.pid == IDLE_PID ? "idle" : ("P" + std::to_string(slice.pid));
        std::cout << "  [" << std::setw(4) << slice.start << " - "
                   << std::setw(4) << slice.end << "]  " << label << "\n";
    }
}

std::vector<int> parse_int_list(const std::string& s) {
    std::vector<int> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) out.push_back(std::stoi(item));
    return out;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                   << " <workload.csv> [--quantum N] [--mlfq-quanta a,b,c] [--gantt]\n";
        return 1;
    }

    std::string workload_path = argv[1];
    int rr_quantum = 4;
    std::vector<int> mlfq_quanta = {4, 8, 16};
    bool show_gantt = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quantum" && i + 1 < argc) {
            rr_quantum = std::stoi(argv[++i]);
        } else if (arg == "--mlfq-quanta" && i + 1 < argc) {
            mlfq_quanta = parse_int_list(argv[++i]);
        } else if (arg == "--gantt") {
            show_gantt = true;
        }
    }

    std::vector<Process> processes;
    try {
        processes = load_workload(workload_path);
    } catch (const std::exception& e) {
        std::cerr << "Error loading workload: " << e.what() << "\n";
        return 1;
    }

    if (processes.empty()) {
        std::cerr << "Workload file contains no processes.\n";
        return 1;
    }

    std::cout << "Loaded " << processes.size() << " processes from " << workload_path << "\n\n";

    std::vector<SimResult> results;
    results.push_back(run_fcfs(processes));
    results.push_back(run_sjf(processes));
    results.push_back(run_srtf(processes));
    results.push_back(run_round_robin(processes, rr_quantum));
    results.push_back(run_priority(processes));
    results.push_back(run_mlfq(processes, mlfq_quanta));

    print_metrics_table(results);

    if (show_gantt) {
        for (const auto& r : results) print_gantt(r);
    }

    return 0;
}
