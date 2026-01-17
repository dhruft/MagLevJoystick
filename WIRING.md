# MagLev Teensy 4.1 Wiring

## Electromagnets (BTS7960)
| Magnet | RPWM (Teensy) | LPWM (Teensy) | Notes |
| :--- | :--- | :--- | :--- |
| **Front** | 37 | 36 | Y-axis Positive |
| **Back** | 23 | 22 | Y-axis Negative |

## TCRT5000 Sensors (4-Axis)
| Sensor | Pin (Teensy) | Axis |
| :--- | :--- | :--- |
| **Sensor 1** | 24 (A10) | 45° |
| **Sensor 2** | 25 (A11) | 135° |
| **Sensor 3** | 16 (A2) | 225° |
| **Sensor 4** | 17 (A3) | 315° |

## Note on EN Pins
- Tie all `L_EN` and `R_EN` pins on the BTS7960 drivers to Teensy **3.3V**.
- Power the TCRT5000 sensors from Teensy **3.3V**.
