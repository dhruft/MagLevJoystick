import serial
import time
import threading

class MagLevJoystick:
    """
    Python API for the ESP32 MagLev Haptic Joystick.
    """
    def __init__(self, port='/dev/cu.usbserial-0001', baudrate=115200):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=0.1)
            time.sleep(2)  # Wait for ESP32 reset
            self._position = 0
            self._running = True
            
            # Start background thread to read position
            self._thread = threading.Thread(target=self._read_loop, daemon=True)
            self._thread.start()
            print(f"Connected to MagLev on {port}")
        except Exception as e:
            print(f"Error connecting to MagLev: {e}")
            self.ser = None

    def _read_loop(self):
        while self._running and self.ser:
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line.startswith("POS:"):
                    self._position = int(line.split(":")[1])
            except:
                continue

    def get_position(self):
        """Returns the normalized position (-100 to 100)"""
        return self._position

    def set_force(self, force):
        """Applies a force (-255 to 255)"""
        if self.ser:
            msg = f"SETF:{int(force)}\n"
            self.ser.write(msg.encode('utf-8'))

    def close(self):
        self._running = False
        if self.ser:
            self.set_force(0)
            self.ser.close()

if __name__ == "__main__":
    # Example Usage:
    joy = MagLevJoystick()
    try:
        while True:
            pos = joy.get_position()
            print(f"Current Position: {pos}")
            
            # Simple "Spring" effect back to center
            force = -pos * 2 
            joy.set_force(force)
            
            time.sleep(0.05)
    except KeyboardInterrupt:
        joy.close()
