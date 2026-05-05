# ardu-picogame

A custom PCB handheld game device based on the Raspberry Pi Pico (RP2040).
Goal: port the games from [ardu-game](../ardu-game) (originally on Seeed XIAO RP2350) to this PCB.

## Hardware

| Component | Part |
|-----------|------|
| MCU | Raspberry Pi Pico (RP2040) |
| Display | ST7789 SPI LCD, 240×240 |
| Buttons | 74HC165 parallel-in shift register (8 inputs) |
| Audio | PWM → audio amplifier (AMP_EN on GP11) |
| Power switch | Peripheral load switch TCK107AF (GP12, active-HIGH) |
| Battery ADC | Resistor divider → GP28/ADC2 |
| I2C expansion | GP8 (SDA), GP9 (SCL) |

Full pin assignment: see **WIRING.md**.

## Software setup

- **Arduino core:** [arduino-pico (Earle Philhower)](https://github.com/earlephilhower/arduino-pico)
- **Board:** Raspberry Pi Pico
- **Libraries:** Adafruit GFX, Adafruit ST7789

Install the Philhower core via Arduino Board Manager URL:
```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

## Sketches

### `pcb_test/`

Hardware bring-up test. Verifies all peripherals on boot and in the main loop:

- **LCD** — splash screen then live status display
- **Buttons** — all 8 inputs shown with green highlight when pressed; raw 165 byte in hex
- **Power button** — shown as PRESSED/released
- **Audio** — each button plays a distinct note via PWM (duty-cycle controlled volume)
- **Battery ADC** — raw ADC value and computed voltage

Serial output at **115200 baud** mirrors button/power events.

**Tunable constants at the top of the sketch:**

| Constant | Default | Purpose |
|----------|---------|---------|
| `BAT_DIVIDER_RATIO` | `2.0` | Match to your resistor divider (R1+R2)/R2 |
| `AUDIO_DUTY` | `24` | PWM duty 0–255; lower = quieter |
| `BTN_*` bit positions | 0–7 | Map to actual 74HC165 A–H inputs on your PCB |

## Hardware notes

- **GP12 (TCK107AF CTRL) must be driven HIGH** before using any peripheral. It gates the 3.3V supply to the LCD, buttons, and amp. Setting it LOW cuts all peripheral power.
- **LCD polarity:** connector Pin 1 = 3V3, Pin 2 = GND. Reversed polarity destroys the display instantly.
- **SPI pins** must be remapped before `tft.init()` — call `SPI.setSCK(18); SPI.setTX(19); SPI.setRX(16);` first (Philhower core does not default to these GPIO).
- **GPIO vs physical pin numbers:** Arduino code uses GPIO numbers (GP0 = `0`), not the 1–40 physical pin numbers on the Pico datasheet.
- **Audio volume:** `tone()` outputs a full 3.3V 50% square wave which clips the amp. Use `analogWriteFreq()` + `analogWrite(pin, duty)` with a low duty cycle instead.
