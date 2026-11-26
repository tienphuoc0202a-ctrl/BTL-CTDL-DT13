CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = print_spooler
TEST_TARGET = test_spooler

all: $(TARGET)

$(TARGET): src/main.cpp src/PrintJob.h src/CircularQueue.h src/LinkedQueue.h src/Algorithms.h src/PrintSpooler.h
	$(CXX) $(CXXFLAGS) src/main.cpp -o $(TARGET)

test: tests/test_spooler.cpp src/PrintJob.h src/CircularQueue.h src/LinkedQueue.h src/Algorithms.h src/PrintSpooler.h
	$(CXX) $(CXXFLAGS) tests/test_spooler.cpp -o $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) data/test_state.csv data/test_report.txt
