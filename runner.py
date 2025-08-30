import subprocess
import time
import json
import os
import pandas as pd
import numpy as np

# Load config to get num_repetitions
with open('config.json', 'r') as f:
    config = json.load(f)

NUM_REPETITIONS = config.get('num_repetitions', 5)
K_VALUES = [1, 5, 10, 20, 50, 100, 200] # Values of k to test
RESULTS_FILE = 'results.csv'

def run_experiment():
    """
    Runs the client-server experiment for various k values and saves results.
    """
    results = []
    
    for k in K_VALUES:
        print(f"--- Testing with k = {k} ---")
        times_for_k = []
        for i in range(NUM_REPETITIONS):
            # Start server in the background
            server_process = subprocess.Popen(['./server'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            # Give server a moment to start
            time.sleep(0.5)

            # Run client
            try:
                # Pass k as a command-line argument
                client_process = subprocess.run(
                    ['./client', str(k)], 
                    capture_output=True, 
                    text=True, 
                    timeout=30 # 30-second timeout
                )
                
                # Find the completion time in the client's output
                completion_time = -1
                for line in client_process.stdout.splitlines():
                    if "Completion Time:" in line:
                        completion_time = float(line.split(':')[1].strip().split(' ')[0])
                        break
                
                if completion_time != -1:
                    times_for_k.append(completion_time)
                    print(f"  Run {i+1}/{NUM_REPETITIONS}: {completion_time:.2f} ms")
                else:
                    print(f"  Run {i+1}/{NUM_REPETITIONS}: Failed to get completion time.")
                    print("Client stdout:", client_process.stdout)
                    print("Client stderr:", client_process.stderr)

            except subprocess.TimeoutExpired:
                print(f"  Run {i+1}/{NUM_REPETITIONS}: Client timed out.")
            finally:
                # Stop the server
                server_process.terminate()
                server_process.wait()

        if times_for_k:
            avg_time = np.mean(times_for_k)
            std_dev = np.std(times_for_k)
            # 95% CI: z * (std_dev / sqrt(n)) where z=1.96 for 95%
            conf_interval = 1.96 * (std_dev / np.sqrt(len(times_for_k))) if len(times_for_k) > 1 else 0
            results.append({'k': k, 'avg_time': avg_time, 'ci': conf_interval})

    # Save results to CSV
    df = pd.DataFrame(results)
    df.to_csv(RESULTS_FILE, index=False)
    print(f"\nResults saved to {RESULTS_FILE}")
    return df

def plot_results():
    """
    Calls the plotting script.
    """
    print("Generating plot...")
    subprocess.run(['python3', 'plot.py'])

if __name__ == "__main__":
    if not (os.path.exists('server') and os.path.exists('client')):
        print("Executables not found. Please run 'make build' first.")
    else:
        run_experiment()
        plot_results()