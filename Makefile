# Makefile for Part 1 - Word Counting Client/Server

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGETS = client server

# Default target
all: build

# Build both client and server
build: $(TARGETS)

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

server: server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp

# Run a single iteration of client-server
run: build
	@echo "Starting server in background..."
	@./server &
	@SERVER_PID=$$!; \
	sleep 1; \
	echo "Running client..."; \
	./client; \
	echo "Stopping server..."; \
	kill $$SERVER_PID 2>/dev/null || true

# Run analysis and generate plot
plot: build
	@echo "Running analysis with varying k values..."
	python3 runner.py

# Clean compiled files
clean:
	rm -f $(TARGETS) results.csv p1_plot.png

.PHONY: all build run plot clean