# MagLev Wiring Guide (Refined: BTS7960 & TCRT5000 Array)

## Electromagnets (BTS7960 Drivers)
Each electromagnet uses a BTS7960 driver, allowing for bi-directional control (useful if polarity switching is needed, though usually MagLev attracts/repels).

| Magnet | RPWM Pin | LPWM Pin | GPIOs | Orientation |
|--------|----------|----------|-------|-------------|
| Front  | 19       | 21       | 19, 21| 0°          |
| Back   | 5        | 18       | 5, 18 | 180°        |

*Note: BTS7960 also typically requires L_EN and R_EN pins tied to 5V or ESP32 GPIOs.*

## TCRT5000 Infrared Sensors (45° CCW Offset)
| Sensor   | ESP32 Pin | GPIO | Relative to |
|----------|-----------|------|-------------|
| Sensor 1 | VP        | 36   | Front CCW 45° |
| Sensor 2 | VN        | 39   | Back CCW 45°  |

## Connection Notes
- **Common Ground**: All GND pins must be shared between ESP32 and Power Supplies.
- **TCRT5000 Logic**: IR sensors are powered by 3.3V to match ESP32 ADC range.
- **BTS7960 Power**: RPWM/LPWM are PWM signals. **CAUTION**: Ensure the BTS7960 VCC (logic) is compatible with ESP32 3.3V (some require 5V logic).
