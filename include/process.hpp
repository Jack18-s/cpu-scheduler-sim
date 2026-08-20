#pragma once
#include <string>
#include <vector>

// Represents a single CPU-bound job to be scheduled.
// remaining_time is mutated during simulation; the other *_time fields
// (first_run_time, completion_time) are filled in as the process is scheduled.
struct Process {
    int pid;
    int arrival_time;
    int burst_time;     // total CPU time required
    int priority;        // lower value = higher priority (used by Priority & MLFQ)

    int remaining_time;   // ticks left to finish
    int first_run_time;   // first tick the process actually got the CPU (-1 = never ran yet)
    int completion_time;  // tick at which the process finished (-1 = not finished)

    Process(int pid_, int arrival_, int burst_, int priority_ = 0)
        : pid(pid_),
          arrival_time(arrival_),
          burst_time(burst_),
          priority(priority_),
          remaining_time(burst_),
          first_run_time(-1),
          completion_time(-1) {}

    int turnaround_time() const { return completion_time - arrival_time; }
    int waiting_time() const { return turnaround_time() - burst_time; }
    int response_time() const { return first_run_time - arrival_time; }
};

// One contiguous slice of CPU time given to a single process. Consecutive
// slices for the same pid are NOT merged, so a Gantt chart can show every
// preemption/resumption exactly as it happened.
struct ScheduleSlice {
    int pid;
    int start;
    int end; // exclusive
};

// Loads processes from a CSV file with header: pid,arrival_time,burst_time,priority
// priority column is optional; defaults to 0 if the file only has 3 columns.
std::vector<Process> load_workload(const std::string& path);
