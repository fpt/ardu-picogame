/*
 * pcb_test.ino — Hardware test for ardu-picogame PCB
 * Target: Raspberry Pi Pico (RP2040)
 * Core:   Earle Philhower arduino-pico
 * Libs:   Adafruit_GFX, Adafruit_ST7789
 *
 * Tests:
 *   - ST7789 240x240 LCD (SPI0)
 *   - 74HC165 shift register (8 buttons)
 *   - Power button (GP5)
 *   - Audio PWM + amplifier enable
 *   - Battery ADC (GP28/ADC2)
 *
 * Serial output at 115200 baud mirrors the display for debugging.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ── Pin definitions (GPIO numbers, Philhower core) ───────────────────────────
// 74HC165 shift register
#define BTN_DATA  2   // GP2,  Pin 4  — Q7 serial output
#define BTN_CLK   3   // GP3,  Pin 5  — CP clock
#define BTN_PL    4   // GP4,  Pin 6  — /PL parallel load (active-low)

#define PWR_BTN   5   // GP5,  Pin 7  — power button (active-low)

#define AMP_EN    11  // GP11, Pin 15 — amplifier enable (HIGH = on)
#define LCD_SD_EN 12  // GP12, Pin 16 — SD card CS (keep HIGH to deselect)
#define AUDIO_PWM 14  // GP14, Pin 19 — PWM audio out

// SPI0 — LCD
#define LCD_MISO  16  // GP16, Pin 21 — SPI0 RX (SD card only, LCD write-only)
#define LCD_CS    17  // GP17, Pin 22 — SPI0 CSn
#define LCD_CLK   18  // GP18, Pin 24 — SPI0 SCK
#define LCD_MOSI  19  // GP19, Pin 25 — SPI0 TX
#define LCD_DC    20  // GP20, Pin 26 — data/command select
#define LCD_RST   21  // GP21, Pin 27 — reset
#define LCD_BL    22  // GP22, Pin 29 — backlight (HIGH = on)

#define BAT_ADC   28  // GP28/ADC2, Pin 34 — battery voltage divider

// ── Adjust to match your PCB button wiring (which input of 165 = which button)
// 74HC165 shifts out H first (bit 7), then G … A (bit 0). Active-low.
#define BTN_UP     0
#define BTN_DOWN   1
#define BTN_LEFT   2
#define BTN_RIGHT  3
#define BTN_START  4
#define BTN_SELECT 5
#define BTN_A      6
#define BTN_B      7

// ── Battery divider ratio — adjust if your PCB uses a different ratio ────────
// Example: 100k + 100k divider → ratio 2.0 (measures up to 6.6V for LiPo)
// If BAT_ADC is wired directly (3.3V max): ratio = 1.0
#define BAT_DIVIDER_RATIO  2.0f

// Audio duty cycle: 0–255 (128 = 50% = loudest). Lower = quieter.
#define AUDIO_DUTY  24

// ── Colours ──────────────────────────────────────────────────────────────────
#define COL_BG      ST77XX_BLACK
#define COL_ACTIVE  ST77XX_GREEN
#define COL_IDLE    0x4208  // dark grey
#define COL_TEXT    ST77XX_WHITE
#define COL_LABEL   ST77XX_CYAN
#define COL_WARN    ST77XX_YELLOW

// ── LCD ──────────────────────────────────────────────────────────────────────
Adafruit_ST7789 tft(LCD_CS, LCD_DC, LCD_RST);

// ── 74HC165 read ─────────────────────────────────────────────────────────────
// Returns 8-bit parallel input: bit 7 = H input, bit 0 = A input.
// Buttons are active-low: 0 = pressed, 1 = released.
static uint8_t read165() {
    // Latch parallel inputs
    digitalWrite(BTN_PL, LOW);
    delayMicroseconds(1);
    digitalWrite(BTN_PL, HIGH);
    // Shift out 8 bits; first bit already on Q7 after latch
    uint8_t val = 0;
    for (int i = 7; i >= 0; i--) {
        val |= (uint8_t)(digitalRead(BTN_DATA) << i);
        digitalWrite(BTN_CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(BTN_CLK, LOW);
        delayMicroseconds(1);
    }
    return val;
}

static inline bool pressed(uint8_t state, uint8_t bit) {
    return !(state & (1u << bit));
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(400);
    Serial.println("=== ardu-picogame PCB test ===");

    // 74HC165
    pinMode(BTN_DATA, INPUT);
    pinMode(BTN_CLK,  OUTPUT);
    pinMode(BTN_PL,   OUTPUT);
    digitalWrite(BTN_CLK, LOW);
    digitalWrite(BTN_PL,  HIGH);

    // Power button
    pinMode(PWR_BTN, INPUT_PULLUP);

    // Amplifier (enable now so startup beep works)
    pinMode(AMP_EN, OUTPUT);
    digitalWrite(AMP_EN, HIGH);

    // GP12 = TCK107AF load switch enable, active-HIGH.
    // HIGH = 3.3V power ON to all peripherals (LCD, buttons, amp).
    pinMode(LCD_SD_EN, OUTPUT);
    digitalWrite(LCD_SD_EN, HIGH);

    // Remap SPI0 to LCD pins before tft.init()
    SPI.setSCK(LCD_CLK);
    SPI.setTX(LCD_MOSI);
    SPI.setRX(LCD_MISO);

    tft.init(240, 240);
    tft.setSPISpeed(40000000);
    tft.setRotation(2);
    tft.fillScreen(ST77XX_RED); // bright red — any pixels = SPI is working

    // Splash screen
    tft.setTextColor(COL_TEXT);
    tft.setTextSize(2);
    tft.setCursor(20, 90);
    tft.print("ardu-picogame");
    tft.setCursor(55, 114);
    tft.setTextSize(1);
    tft.setTextColor(COL_LABEL);
    tft.print("PCB test — ok");
    delay(600);

    // Startup beep: two rising tones
    analogWriteFreq(880);  analogWrite(AUDIO_PWM, AUDIO_DUTY); delay(100);
    analogWriteFreq(1760); analogWrite(AUDIO_PWM, AUDIO_DUTY); delay(100);
    analogWrite(AUDIO_PWM, 0);

    tft.fillScreen(COL_BG);

    // Draw static labels (only once)
    tft.setTextSize(1);
    tft.setTextColor(COL_LABEL);
    tft.setCursor(2,  2);  tft.print("BUTTONS (165)");
    tft.setCursor(2, 42);  tft.print("PWR BTN");
    tft.setCursor(2, 62);  tft.print("BATTERY");
    tft.setCursor(2, 82);  tft.print("RAW 165");

    Serial.println("Setup done. Monitoring...");
}

// ── Loop ─────────────────────────────────────────────────────────────────────
static uint8_t prevBtns   = 0xFF;
static bool    prevPwr    = false;

void loop() {
    uint8_t btns = read165();
    bool    pwr  = (digitalRead(PWR_BTN) == LOW);

    // Battery: ADC → voltage
    int   adcRaw = analogRead(BAT_ADC);
    float batV   = adcRaw * (3.3f / 1023.0f) * BAT_DIVIDER_RATIO;

    // ── Update display ────────────────────────────────────────────────────
    // Buttons row
    struct { const char* lbl; uint8_t bit; } bmap[] = {
        {"UP", BTN_UP}, {"DN", BTN_DOWN}, {"LT", BTN_LEFT}, {"RT", BTN_RIGHT},
        {"A",  BTN_A},  {"B",  BTN_B},   {"ST", BTN_START}, {"SE", BTN_SELECT}
    };
    tft.setTextSize(1);
    tft.setCursor(2, 14);
    for (auto& b : bmap) {
        bool p = pressed(btns, b.bit);
        tft.setTextColor(p ? COL_ACTIVE : COL_IDLE, COL_BG);
        tft.print(b.lbl);
        tft.print(" ");
    }

    // Raw byte
    tft.setCursor(2, 94);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.print("0x");
    if (btns < 0x10) tft.print("0");
    tft.print(btns, HEX);
    tft.print("  ");

    // Power button
    tft.setCursor(2, 54);
    tft.setTextColor(pwr ? COL_WARN : COL_IDLE, COL_BG);
    tft.print(pwr ? "PRESSED   " : "released  ");

    // Battery
    tft.setCursor(2, 74);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.print(batV, 2);
    tft.print("V  ADC=");
    tft.print(adcRaw);
    tft.print("   ");

    // ── Serial log on change ──────────────────────────────────────────────
    if (btns != prevBtns) {
        Serial.print("165=0x"); Serial.print(btns, HEX);
        Serial.print("  pressed:");
        for (auto& b : bmap) {
            if (pressed(btns, b.bit)) {
                Serial.print(" "); Serial.print(b.lbl);
            }
        }
        Serial.println();
        prevBtns = btns;
    }
    if (pwr != prevPwr) {
        Serial.println(pwr ? "PWR BTN: pressed" : "PWR BTN: released");
        prevPwr = pwr;
    }

    // ── Audio: each button plays a distinct note ───────────────────────────
    {
        uint32_t freq = 0;
        if      (pressed(btns, BTN_A))      freq = 440;
        else if (pressed(btns, BTN_B))      freq = 494;
        else if (pressed(btns, BTN_UP))     freq = 523;
        else if (pressed(btns, BTN_DOWN))   freq = 587;
        else if (pressed(btns, BTN_LEFT))   freq = 659;
        else if (pressed(btns, BTN_RIGHT))  freq = 698;
        else if (pressed(btns, BTN_START))  freq = 784;
        else if (pressed(btns, BTN_SELECT)) freq = 880;

        if (freq) { analogWriteFreq(freq); analogWrite(AUDIO_PWM, AUDIO_DUTY); }
        else       analogWrite(AUDIO_PWM, 0);
    }

    delay(20);
}
