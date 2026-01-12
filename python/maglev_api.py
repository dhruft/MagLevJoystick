import serial
import time
import threading

class MagLevJoystick2D:
    """
    Python API for the 2D XY MagLev Haptic Joystick.
    """
    def __init__(self, port='/dev/cu.usbserial-0001', baudrate=115200):
        self._x = 0
        self._y = 0
        self._running = True
        try:
            self.ser = serial.Serial(port, baudrate, timeout=0.1)
            time.sleep(2)
            self._thread = threading.Thread(target=self._read_loop, daemon=True)
            self._thread.start()
            print(f"Connected to 2D MagLev on {port}")
        except Exception as e:
            print(f"Connection failed: {e}")
            self.ser = None

    def _read_loop(self):
        while self._running and self.ser:
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line.startswith("POS:"):
                    raw = line.split(":")[1].split(",")
                    self._x = int(raw[0])
                    self._y = int(raw[1])
            except:
                continue

    def get_position(self):
        """Returns (x, y) coordinates from -100 to 100"""
        return (self._x, self._y)

    def set_force(self, fx, fy):
        """Applies force vector (-255 to 255 for each axis)"""
        if self.ser:
            msg = f"SETF:{int(fx)},{int(fy)}\n"
            self.ser.write(msg.encode('utf-8'))

    def close(self):
        self._running = False
        if self.ser:
            self.set_force(0, 0)
            self.ser.close()

if __name__ == "__main__":
    joy = MagLevJoystick2D()
    try:
        while True:
            x, y = joy.get_position()
            print(f"Position -> X: {x:4}, Y: {y:4}")
            
            # Interactive centering force (Spring Effect)
            joy.set_force(-x * 2, -y * 2)
            
            time.sleep(0.05)
    except KeyboardInterrupt:
        joy.close()
