# CLAUDE.md — ardu-picogame

## Project goal

Port the games from `../ardu-game` (Seeed XIAO RP2350) to this custom Raspberry Pi Pico PCB.
The hardware differs significantly — see the mapping section below.

## Arduino environment

- **Core:** Earle Philhower arduino-pico
- **Board:** Raspberry Pi Pico (RP2040)
- **Pin numbers in code = GPIO numbers** (GP0 = `0`, GP18 = `18`, etc.)
  Physical pin numbers (1–40 on the Pico datasheet) are different and must not be used in code.

## Key hardware differences vs ardu-game (XIAO RP2350)

| Feature | ardu-game (XIAO RP2350) | ardu-picogame (Pico PCB) |
|---------|------------------------|--------------------------|
| Buttons | Adafruit seesaw I2C gamepad | 74HC165 shift register, bit-bang SPI |
| Button read | `pad.digitalReadBulk()` | `read165()` in pcb_test |
| SPI LCD pins | GPIO2/3 (SCK/MOSI) | GP18/GP19 (SCK/MOSI) |
| Audio | `PwmSound` library | `analogWriteFreq()` + `analogWrite()` |
| Peripheral power | Always on | TCK107AF load switch on GP12 (active-HIGH) |
| I2C | Wire1, GPIO6/GPIO7 | Wire, GP8/GP9 (i2c0) |

## Critical hardware quirks (learned during bring-up)

**GP12 (LCD_SD_EN) must be HIGH before anything else.**
It drives the TCK107AF load switch CTRL pin (active-HIGH), which gates the 3.3V rail to all peripherals. Setting it LOW kills the LCD, buttons, and amp simultaneously. Always set it HIGH at the very top of `setup()`.

**SPI must be remapped before `tft.init()`.**
Call `SPI.setSCK(18); SPI.setTX(19); SPI.setRX(16);` before `tft.init()`. The Philhower core does not default SPI0 to these GPIO.

**Do not use `tone()` for audio.**
`tone()` outputs a 50% duty cycle square wave at full 3.3V, which clips the amplifier.
Use `analogWriteFreq(freq); analogWrite(AUDIO_PWM, duty);` with a low duty value (e.g. 24) instead.
To stop audio: `analogWrite(AUDIO_PWM, 0)`.

**74HC165 bit order:** after a /PL latch pulse, Q7 immediately reflects the H input (bit 7). Each clock shifts the next bit. So the first bit read = H = bit 7, last = A = bit 0. Buttons are active-low.

**LCD polarity:** Pin 1 = 3V3, Pin 2 = GND on the LCD connector. Swapping destroys the display.

## File map

```
WIRING.md          — full pin assignment with GPIO numbers, connector tables, hardware notes
CLAUDE.md          — this file
README.md          — project overview and setup instructions
pcb_test/
  pcb_test.ino     — hardware bring-up test (LCD, buttons, audio, ADC, power button)
```

## When porting ardu-game sketches

1. Replace all `pad.digitalReadBulk()` / seesaw calls with `read165()`.
2. Replace `Wire1` with `Wire`; call `Wire.setSDA(8); Wire.setSCL(9);` before `Wire.begin()`.
3. Replace SPI pin defines with the values in WIRING.md.
4. Replace `PwmSound` / `tone()` with `analogWriteFreq()` + `analogWrite()`.
5. Add GP12 HIGH at the start of `setup()` before any peripheral init.
6. Check button bit assignments — the 74HC165 A–H inputs may be wired in a different order than assumed in pcb_test. Verify with Serial output and adjust the `BTN_*` defines.
