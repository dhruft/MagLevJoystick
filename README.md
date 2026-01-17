## 🏗 System Architecture

The project is split into two distinct layers that talk over USB:

1.  **Device Layer (C++ / Teensy 4.1)**: Runs directly on the Teensy hardware. It handles the "Real-Time" magnetic stabilization (kHz loop), reading sensors, and driving the electromagnets.
2.  **Application Layer (Python / Laptop)**: Runs on your computer. This is where your game logic lives. It sends "Goal Forces" to the Teensy and receives "Current Position" data.

---

## 🚀 Orientation for Python Developers

You can start coding your game **immediately**, even without the physical hardware, using **Mock Mode**.

### 1. Setup
```bash
pip install pyserial
```

### 2. The API Instance
```python
from maglev_api import MagLevJoystick

# IF YOU HAVE THE HARDWARE:
joy = MagLevJoystick(port='/dev/cu.usbserial-0001')

# IF YOU ARE CODING REMOTELY (NO HARDWARE):
joy = MagLevJoystick(mock=True)
```

### 3. Core Functions
| Function | Description | Range |
| :--- | :--- | :--- |
| `get_position()` | Returns `(x, y)` tuple. | `-100` to `100` per axis |
| `set_force(fx, fy)` | Applies haptic feedback. | `-255` to `255` per axis |

---

## 📐 Coordinate System & Physics

The joystick follows a standard Cartesian plane:
- **X-Axis**: `-100` (Left) to `+100` (Right).
- **Y-Axis**: `-100` (Back/Down) to `+100` (Front/Up).
- **Center**: `(0, 0)`.

### 🧲 Applying Haptics
Forces are cumulative. To create a **"Spring to Center"** effect:
```python
x, y = joy.get_position()
joy.set_force(-x * 2, -y * 2) # Pulls the magnet back to (0,0)
```

To create a **"Vibration/Rumble"**:
```python
import random
joy.set_force(random.randint(-50, 50), random.randint(-50, 50))
```

---

## ⚡ Technical Architecture (High Speed)
This system is designed for sub-millisecond latency to ensure stable magnetic levitation and crisp haptic feel:
- **ESP32 Dual-Core**: Core 1 handles the 2kHz magnetic control loop; Core 0 handles Serial comms.
- **Async Python**: The API uses separate background threads for reading and writing, so your game loop never stutters waiting for Serial I/O.
- **Mock Physics**: In `mock` mode, the API simulates inertia, friction, and force integration so your haptic logic feels realistic during development.

---

## 📂 Project Structure
- `python/maglev_api.py`: The only file you need to import.
- `src/main.cpp`: ESP32 Firmware (PlatformIO).
- `WIRING.md`: Hardware schematics.
