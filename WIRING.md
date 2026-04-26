# Wiring Reference — ardu-picogame PCB

## Raspberry Pi Pico Pin Assignment

> ⚠️ **Pin numbering:** The table uses **physical pin numbers** (1–40) from the
> Pico datasheet. In Arduino (Philhower core) code, use the **GPIO number**, not
> the physical pin number. They are different — e.g. physical pin 1 = GP0 = `0` in code.

```
                      Raspberry Pi Pico
                    ┌──────────────────┐
  UART_TX ─── GP0 ──┤ 1            40 ├── VBUS (5V)
  UART_RX ─── GP1 ──┤ 2            39 ├── VSYS
              GND ──┤ 3            38 ├── GND
 BTN165_DATA  GP2 ──┤ 4            37 ├── 3V3_EN
 BTN165_CLK   GP3 ──┤ 5            36 ├── 3V3
 BTN165_PL    GP4 ──┤ 6            35 ├── ADC_VREF
  PWR_BTN     GP5 ──┤ 7            34 ├── GP28/ADC2 ── BAT_ADC
              GND ──┤ 8            33 ├── AGND
                   ──┤ 9            32 ├── GP27
  I2C_SDA     GP8 ──┤11            31 ├── GP26
  I2C_SCL     GP9 ──┤12            30 ├── RUN
              GND ──┤13            29 ├── GP22 ── LCD_BL
                   ──┤14            28 ├── GND
  AMP_EN      GP11──┤15            27 ├── GP21 ── LCD_RST
  LCD_SD_EN   GP12──┤16            26 ├── GP20 ── LCD_DC
                   ──┤17            25 ├── GP19 ── LCD_MOSI (SPI0 TX)
  AUDIO_PWM   GP14──┤19            24 ├── GP18 ── LCD_CLK  (SPI0 SCK)
                   ──┤20            23 ├── GND
  LCD_MISO    GP16──┤21            22 ├── GP17 ── LCD_CS
                    └──────────────────┘
```

---

## 1. ST7789 SPI LCD (240×240)

> ⚠️ **Polarity warning:** Pin 1 = 3V3, Pin 2 = GND. Swapping these destroys the LCD instantly. Double-check before powering on.

| LCD Pin | Symbol   | → | Pico Physical | GPIO  | Code constant   |
|---------|----------|---|---------------|-------|-----------------|
| 1       | 3V3      | → | 36            | 3V3   | —               |
| 2       | GND      | → | 38            | GND   | —               |
| 3       | CLK/SCK  | → | 24            | GP18  | `LCD_CLK  18`   |
| 4       | MOSI/SDA | → | 25            | GP19  | `LCD_MOSI 19`   |
| 5       | RST/RES  | → | 27            | GP21  | `LCD_RST  21`   |
| 6       | DC       | → | 26            | GP20  | `LCD_DC   20`   |
| 7       | CS       | → | 22            | GP17  | `LCD_CS   17`   |
| 8       | BL       | → | 29            | GP22  | `LCD_BL   22`   |

> **SPI0** hardware peripheral: SCK=GP18, TX=GP19, RX=GP16, CSn=GP17.
> Call `SPI.setSCK(18); SPI.setTX(19); SPI.setRX(16);` before `tft.init()`.

> **LCD_MISO (GP16, Pin 21)** is unused by the LCD itself (write-only display).
> It is wired for the shared SD card on the same SPI bus.

---

## 2. 74HC165 Shift Register (Buttons)

| 74HC165 Pin | Symbol | → | Pico Physical | GPIO | Code constant  |
|-------------|--------|---|---------------|------|----------------|
| 1           | /PL    | → | 6             | GP4  | `BTN_PL   4`   |
| 2           | CP     | → | 5             | GP3  | `BTN_CLK  3`   |
| 9           | Q7     | → | 4             | GP2  | `BTN_DATA 2`   |
| 8, 16       | GND    | → | GND           | —    | —              |
| 16          | VCC    | → | 36            | 3V3  | —              |

> **Active-low buttons:** wire each button between a 74HC165 input pin and GND;
> enable the 165's internal pull-up via the VCC connection or add 10 kΩ
> pull-ups externally. Pressed = input LOW = bit = 0.

> **Read protocol:** pulse /PL LOW (≥100 ns) then HIGH to latch parallel inputs.
> Then clock CP 8 times, sampling Q7 before each rising edge. First bit out = H
> input (bit 7); last bit out = A input (bit 0).

---

## 3. Audio

| Component  | Signal    | → | Pico Physical | GPIO | Code constant  |
|------------|-----------|---|---------------|------|----------------|
| Amp input  | AUDIO_PWM | → | 19            | GP14 | `AUDIO_PWM 14` |
| Amp enable | AMP_EN    | → | 15            | GP11 | `AMP_EN    11` |

> Drive `AMP_EN` HIGH to enable the amplifier, LOW to mute/power-down.
> Use `tone(AUDIO_PWM, freq)` / `noTone(AUDIO_PWM)` in Arduino.

---

## 4. Power Button

| Signal  | → | Pico Physical | GPIO | Code constant |
|---------|---|---------------|------|---------------|
| PWR_BTN | → | 7             | GP5  | `PWR_BTN 5`   |

> Wire between GP5 and GND. Enable internal pull-up: `pinMode(5, INPUT_PULLUP)`.
> Button pressed = GPIO reads LOW.

---

## 5. Battery ADC

| Signal  | → | Pico Physical | GPIO      | Code constant  |
|---------|---|---------------|-----------|----------------|
| BAT_ADC | → | 34            | GP28/ADC2 | `BAT_ADC  28`  |

> Feed through a resistor divider to keep voltage ≤ 3.3V on the ADC pin.
> Adjust the scale factor in firmware to match your divider ratio.

---

## 6. I2C (Optional / Expansion)

| Signal   | → | Pico Physical | GPIO | Code constant |
|----------|---|---------------|------|---------------|
| I2C_SDA  | → | 11            | GP8  | `I2C_SDA 8`   |
| I2C_SCL  | → | 12            | GP9  | `I2C_SCL 9`   |

> GP8/GP9 map to **i2c0** on the Philhower core.
> Use `Wire.setSDA(8); Wire.setSCL(9); Wire.begin();`.

---

## 7. Peripheral Power Switch (TCK107AF)

| Signal     | → | Pico Physical | GPIO | Code constant    |
|------------|---|---------------|------|------------------|
| CTRL       | → | 16            | GP12 | `LCD_SD_EN  12`  |

> GP12 drives the **CTRL pin of a TCK107AF load switch**, which gates the 3.3V
> supply to all peripherals (LCD, buttons, amp).
> **Active-HIGH**: drive HIGH to power peripherals on, LOW to cut all 3.3V.
> Must be set HIGH before any peripheral (SPI, I2C, buttons) is used.

---

## Full Signal Summary

| Pico Pin | GPIO      | Signal       | Direction |
|----------|-----------|--------------|-----------|
| 1        | GP0       | UART_TX      | Out       |
| 2        | GP1       | UART_RX      | In        |
| 4        | GP2       | BTN165_DATA  | In        |
| 5        | GP3       | BTN165_CLK   | Out       |
| 6        | GP4       | BTN165_PL    | Out       |
| 7        | GP5       | PWR_BTN      | In        |
| 11       | GP8       | I2C_SDA      | I/O       |
| 12       | GP9       | I2C_SCL      | Out       |
| 15       | GP11      | AMP_EN       | Out       |
| 16       | GP12      | PWR_EN (TCK107AF CTRL, active-HIGH) | Out |
| 19       | GP14      | AUDIO_PWM    | Out       |
| 21       | GP16      | LCD_MISO     | In        |
| 22       | GP17      | LCD_CS       | Out       |
| 24       | GP18      | LCD_CLK      | Out       |
| 25       | GP19      | LCD_MOSI     | Out       |
| 26       | GP20      | LCD_DC       | Out       |
| 27       | GP21      | LCD_RST      | Out       |
| 29       | GP22      | LCD_BL       | Out       |
| 34       | GP28/ADC2 | BAT_ADC      | In (ADC)  |

> ⚠️ **Pin 25 (GP19 / LCD_MOSI) was missing from the original schematic notes.**
> It is required for SPI display output — verify it is routed on your PCB.
