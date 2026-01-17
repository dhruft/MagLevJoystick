import matplotlib.pyplot as plt
import csv
import os

FILE_PATH = os.path.join(os.path.dirname(__file__), "sensor_data.csv")

def plot_data():
    if not os.path.exists(FILE_PATH):
        print(f"File {FILE_PATH} not found!")
        return

    s1_vals, s2_vals, t1_vals, t2_vals = [], [], [], []

    print(f"Reading {FILE_PATH}...")
    try:
        with open(FILE_PATH, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    s1_vals.append(float(row['s1']))
                    s2_vals.append(float(row['s2']))
                    t1_vals.append(float(row['t1']))
                    t2_vals.append(float(row['t2']))
                except (ValueError, KeyError):
                    continue
    except Exception as e:
        print(f"Error reading file: {e}")
        return

    if not s1_vals:
        print("No valid data found in CSV.")
        return

    print(f"Plotting {len(s1_vals)} data points...")
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Plot S1 vs T1
    ax1.scatter(t1_vals, s1_vals, alpha=0.5, s=2, c='blue')
    ax1.set_title("IR S1 vs ToF T1")
    ax1.set_xlabel("ToF Distance (mm)")
    ax1.set_ylabel("Raw IR Value (s1)")
    ax1.grid(True)

    # Plot S2 vs T2
    ax2.scatter(t2_vals, s2_vals, alpha=0.5, s=2, c='red')
    ax2.set_title("IR S2 vs ToF T2")
    ax2.set_xlabel("ToF Distance (mm)")
    ax2.set_ylabel("Raw IR Value (s2)")
    ax2.grid(True)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    plot_data()
