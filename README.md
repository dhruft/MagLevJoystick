# MagLev Haptic Joystick

This project uses an ESP32 and electromagnets to create a 1D haptic joystick.

## 🚀 Getting Started for Python Devs

1.  **Install Dependencies**:
    ```bash
    pip install pyserial
    ```
2.  **Import the API**:
    Use `python/maglev_api.py` to interact with the hardware.
    ```python
    from maglev_api import MagLevJoystick
    
    joy = MagLevJoystick2D(port='/dev/cu.usbserial-0001')
    x, y = joy.get_position() # (-100, 100) range
    joy.set_force(50, -20)     # Apply force vector
    ```

## 📂 Project Structure
- `/src`: ESP32 Firmware (XY 4-Magnet)
- `/python`: Python API (now with 2D support)
- `WIRING.md`: 2D Hardware assembly guide

## 🔧 Hardware Specs
- **Sensors**: 4x TCRT5000 IR (Analog)
- **Actuators**: 4x Electromagnets via 2x BTS7960 Dual-Channel or 4x Single
