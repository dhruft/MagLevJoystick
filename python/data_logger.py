import serial
import time
import os

# Configuration
PORT = '/dev/cu.usbmodem189946801'
BAUD = 115200
FILE_PATH = os.path.join(os.path.dirname(__file__), "sensor_data.csv")

def collect_data(port, baud, output_file, duration_seconds=10, sample_limit=None):
    print(f"Connecting to {port}...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
        ser.reset_input_buffer()
    except Exception as e:
        print(f"Error connecting to serial: {e}")
        return False

    print(f"Logging to {output_file}...")
    print(f"Capturing data for {duration_seconds}s (Limit: {sample_limit} samples)...")

    start_time = time.time()
    samples = 0
    try:
        with open(output_file, 'w') as f:
            f.write("s1,s2,t1,t2\n")
            while True:
                elapsed = time.time() - start_time
                if elapsed > duration_seconds:
                    break
                if sample_limit and samples >= sample_limit:
                    break

                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line.count(',') == 3:
                        f.write(line + "\n")
                        samples += 1
                        if samples % 50 == 0:
                            print(f"\rCaptured {samples} samples...", end="")
        print(f"\nFinished. Captured {samples} samples.")
        return True
    except KeyboardInterrupt:
        print("\nLogging interrupted by user.")
        return True
    finally:
        ser.close()

if __name__ == "__main__":
    collect_data(PORT, BAUD, FILE_PATH, duration_seconds=30)