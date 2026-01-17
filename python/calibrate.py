import serial
import time
import os
import sys
from data_logger import collect_data
from correlator import derive_functions

# Configuration
PORT = '/dev/cu.usbmodem189946801'
BAUD = 115200
DATA_FILE = os.path.join(os.path.dirname(__file__), "sensor_data.csv")

def run_calibration():
    print("--- MagLev One-Click Calibration Pipeline ---")
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(1) # wait for connection
    except Exception as e:
        print(f"ERROR: Could not open serial port {PORT}. {e}")
        return

    print("\n1. Enabling Calibration Mode on Teensy...")
    ser.write(b"CAL_ON\n")
    ser.flush()
    
    # Check for confirmation
    confirm = ser.readline().decode().strip()
    print(f"Teensy says: {confirm}")

    print("\n2. Starting Data Collection...")
    print("   MOVE THE MAGNET through its full X/Y range manually or start the sway demo.")
    print("   Capturing will continue for 30 seconds or until you press Ctrl+C.")
    
    ser.close() # Close so data_logger can open it
    
    success = collect_data(PORT, BAUD, DATA_FILE, duration_seconds=30)
    
    if not success:
        print("Data collection failed.")
        return

    # Re-open to turn off cal mode
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        print("\n3. Disabling Calibration Mode...")
        ser.write(b"CAL_OFF\n")
        ser.flush()
        ser.close()
    except:
        print("Warning: Could not disable calibration mode automatically.")

    print("\n4. Deriving Correlation Polynomials & Generating Header...")
    derive_functions(show_plots=True)

    print("\n--- CALIBRATION SUCCESSFUL ---")
    print("The file 'src/virtual_tof_coeffs.h' has been updated.")
    print("👉 ACTION REQUIRED: Re-upload your firmware in PlatformIO to apply the new calibration!")

if __name__ == "__main__":
    run_calibration()
