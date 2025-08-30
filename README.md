# Part 1: Word Counting Client-Server

This is Part 1 of the Socket Programming Assignment. It creates a simple client-server program using TCP sockets in C++.

## What This Program Does

1. **Server**: Reads words from a file and sends them to clients when requested
2. **Client**: Connects to server, downloads words in chunks, and counts how often each word appears
3. **Analysis**: Tests different chunk sizes to see which is fastest

## Files in This Folder

- `client.cpp` - The client program (written in C++)
- `server.cpp` - The server program (written in C++)
- `Makefile` - Commands to build and run the programs
- `config.json` - Settings like server address and port
- `words.txt` - File containing words to download
- `runner.py` - Script to test different chunk sizes
- `plot.py` - Script to create a graph of results

## How to Run

### Step 1: Install Python packages (one time only)
```bash
python3 -m venv venv
source venv/bin/activate
pip install pandas matplotlib numpy
```

### Step 2: Build the programs
```bash
make build
```

### Step 3: Test with one run
```bash
make run
```

### Step 4: Run full analysis and create graph
```bash
make plot
```

## What Each Command Does

- `make build` - Compiles the C++ code into executable programs
- `make run` - Starts server, runs client once, shows word counts
- `make plot` - Tests different chunk sizes and creates a performance graph
- `make clean` - Removes compiled programs and result files

## How It Works

1. **Client sends request**: `p,k` where p=starting position, k=number of words
2. **Server responds**: Sends k words starting from position p
3. **End of file**: Server sends "EOF" when no more words available
4. **Word counting**: Client counts how many times each word appears
5. **Timing**: Client measures how long the whole process takes

## Example Output

When you run `make run`, you'll see:
```
--- Word Frequencies ---
apple, 8
banana, 6
cat, 6
...
------------------------
Completion Time: 0.60 ms
```

## The Analysis Graph

The `make plot` command creates `p1_plot.png` showing:
- **X-axis**: Chunk size (k) - how many words per request
- **Y-axis**: Time to download all words
- **Result**: Smaller chunks take longer because of more network requests

## Key Findings

- **Small chunks (k=1)**: Slow because many network requests needed
- **Large chunks (k=200)**: Fast because fewer network requests
- **Sweet spot**: Around k=50-100 for best performance vs memory usage

