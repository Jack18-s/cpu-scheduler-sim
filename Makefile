CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

SRC := src/process.cpp src/metrics.cpp \
       src/schedulers/fcfs.cpp src/schedulers/sjf.cpp src/schedulers/srtf.cpp \
       src/schedulers/round_robin.cpp src/schedulers/priority.cpp src/schedulers/mlfq.cpp

BIN := scheduler_sim
TEST_BIN := run_tests

.PHONY: all clean test run

all: $(BIN)

$(BIN): src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $(BIN) $^

$(TEST_BIN): tests/test_schedulers.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TEST_BIN) $^

test: $(TEST_BIN)
	./$(TEST_BIN)

run: $(BIN)
	./$(BIN) data/workload1.csv --gantt

clean:
	rm -f $(BIN) $(TEST_BIN)
