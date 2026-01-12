import serial
import time
import threading
import queue

class MagLevJoystickHighSpeed:
    """
    Performance-optimized Python API for the 2D MagLev Haptic Joystick.
    Uses dedicated threads for non-blocking I/O.
    """
    def __init__(self, port='/dev/cu.usbserial-0001', baudrate=460800):
        self._x = 0
        self._y = 0
        self._running = True
        self._mock = mock
        self._write_queue = queue.Queue(maxsize=1)
        
        if self._mock:
            self.ser = None
            self._thread = threading.Thread(target=self._mock_loop, daemon=True)
            self._thread.start()
            print("Running in MOCK mode (Simulated Physics)")
        else:
            try:
                self.ser = serial.Serial(port, baudrate, timeout=0.01)
                time.sleep(1.5) # Wait for boot
                self._read_thread = threading.Thread(target=self._read_loop, daemon=True)
                self._write_thread = threading.Thread(target=self._write_loop, daemon=True)
                self._read_thread.start()
                self._write_thread.start()
                print(f"Connected to High-Speed MagLev on {port}")
            except Exception as e:
                print(f"Hardware connection failed: {e}. Falling back to MOCK mode.")
                self._mock = True
                self.ser = None
                self._thread = threading.Thread(target=self._mock_loop, daemon=True)
                self._thread.start()

    def _read_loop(self):
        """Asynchronous reader thread (Core 0 counterpart)"""
        while self._running and self.ser:
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line.startswith("P:"):
                    raw = line[2:].split(",")
                    self._x, self._y = int(raw[0]), int(raw[1])
            except:
                continue

    def _write_loop(self):
        """Asynchronous writer thread to avoid blocking the game loop"""
        while self._running and self.ser:
            try:
                # Get the latest command, wait if empty
                fx, fy = self._write_queue.get(timeout=0.1)
                msg = f"F:{fx},{fy}\n"
                self.ser.write(msg.encode('utf-8'))
                self.ser.flush()
            except queue.Empty:
                continue
            except:
                break

    def _mock_loop(self):
        """Simulates simple physics for testing games without hardware"""
        vel_x, vel_y = 0, 0
        last_fx, last_fy = 0, 0
        
        while self._running:
            # Get latest force from queue
            try:
                last_fx, last_fy = self._write_queue.get_nowait()
            except queue.Empty:
                pass
            
            # Very simple Euler integration for haptic feel
            # Acceleration from forces
            accel_x = last_fx * 0.01 
            accel_y = last_fy * 0.01
            
            # Friction/Damping
            vel_x = (vel_x + accel_x) * 0.95
            vel_y = (vel_y + accel_y) * 0.95
            
            self._x += vel_x
            self._y += vel_y
            
            # Add some random jitter/noise
            self._x += (random.random() - 0.5) * 0.1
            self._y += (random.random() - 0.5) * 0.1

            # Hard limits
            self._x = max(-100, min(100, self._x))
            self._y = max(-100, min(100, self._y))
            
            time.sleep(0.01) # 100Hz simulation

    def get_position(self):
        """Thread-safe position access"""
        return (int(self._x), int(self._y))

    def set_force(self, fx, fy):
        """Queue a force update without blocking the game thread"""
        if not self._write_queue.full():
            self._write_queue.put_nowait((int(fx), int(fy)))
        else:
            # Drop older command if queue is full to ensure lowest latency
            try:
                self._write_queue.get_nowait()
                self._write_queue.put_nowait((int(fx), int(fy)))
            except:
                pass

    def close(self):
        self._running = False
        if self.ser:
            self.set_force(0, 0)
            time.sleep(0.1)
            self.ser.close()

if __name__ == "__main__":
    # Test Bench: Interactive Spring
    joy = MagLevJoystickHighSpeed()
    try:
        while True:
            x, y = joy.get_position()
            # print(f"X: {x:4}, Y: {y:4}")
            joy.set_force(-x * 2.5, -y * 2.5) # Dynamic resistance
            time.sleep(0.005) # 200fps game loop
    except KeyboardInterrupt:
        joy.close()
