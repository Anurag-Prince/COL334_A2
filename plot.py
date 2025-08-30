import pandas as pd
import matplotlib.pyplot as plt

RESULTS_FILE = 'results.csv'
OUTPUT_PLOT = 'p1_plot.png'

def generate_plot():
    """
    Generates a plot from the results CSV file.
    """
    try:
        df = pd.read_csv(RESULTS_FILE)
    except FileNotFoundError:
        print(f"Error: {RESULTS_FILE} not found. Please run the analysis first.")
        return

    plt.style.use('seaborn-v0_8-whitegrid')
    fig, ax = plt.subplots(figsize=(10, 6))

    ax.errorbar(
        df['k'], 
        df['avg_time'], 
        yerr=df['ci'], 
        fmt='-o', 
        capsize=5, 
        label='Average Completion Time'
    )

    ax.set_xlabel('k (Number of Words per Request)')
    ax.set_ylabel('Average Completion Time (ms)')
    ax.set_title('Effect of Chunk Size (k) on File Download Time')
    ax.set_xticks(df['k'])
    ax.legend()
    
    # Improve readability for x-axis labels if they overlap
    fig.autofmt_xdate(rotation=45)
    plt.tight_layout()

    plt.savefig(OUTPUT_PLOT)
    print(f"Plot saved as {OUTPUT_PLOT}")

if __name__ == "__main__":
    generate_plot()