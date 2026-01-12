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
    
    joy = MagLevJoystick(port='/dev/cu.usbserial-0001')
    pos = joy.get_position() # -100 to 100
    joy.set_force(50)        # Apply feedback force
    ```

## 📂 Project Structure
- `/src`: ESP32 Firmware (PlatformIO)
- `/python`: Python API and Game Examples
- `WIRING.md`: Hardware assembly guide

## 🔧 Hardware Specs
- **Sensor**: TCRT5000 IR (Analog)
- **Actuators**: 2x Electromagnets via BTS7960
- **Range**: ~40mm linear travel
