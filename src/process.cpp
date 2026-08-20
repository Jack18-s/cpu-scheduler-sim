#include "process.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<Process> load_workload(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open workload file: " + path);
    }

    std::vector<Process> processes;
    std::string line;
    bool first_line = true;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Skip header row (detected by non-numeric first field).
        if (first_line) {
            first_line = false;
            if (!line.empty() && !std::isdigit(static_cast<unsigned char>(line[0]))) {
                continue;
            }
        }

        std::stringstream ss(line);
        std::string field;
        std::vector<int> values;
        while (std::getline(ss, field, ',')) {
            values.push_back(std::stoi(field));
        }

        if (values.size() < 3) {
            throw std::runtime_error("Malformed workload row: " + line);
        }

        int pid = values[0];
        int arrival = values[1];
        int burst = values[2];
        int priority = values.size() >= 4 ? values[3] : 0;

        processes.emplace_back(pid, arrival, burst, priority);
    }

    return processes;
}
