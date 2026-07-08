#include <Arduino.h>

// 74HC595 control lines
constexpr uint8_t PIN_DATA = 13;  // D7 -> DS   (595 pin 14)
constexpr uint8_t PIN_CLOCK = 14; // D5 -> SHCP (595 pin 11)
constexpr uint8_t PIN_LATCH = 12; // D6 -> STCP (595 pin 12)

// Digit cathodes DIG1..DIG4, active LOW (direct drive).
// If you add NPN transistors later, flip LOW/HIGH in refresh().
constexpr uint8_t DIGIT_PINS[4] = {5, 4, 16, 15}; // D1, D2, D0, D8

// Buttons: one leg to the pin, other leg to GND. Internal pull-ups keep
// GPIO0/GPIO2 high at boot (required for normal flash boot) and give all
// three a defined idle level, so pressed == LOW.
constexpr uint8_t PIN_BTN_TOGGLE = 0;      // D3 -> toggle running/paused
constexpr uint8_t PIN_BTN_RESET_WORK = 2;  // D4 -> reset to work duration
constexpr uint8_t PIN_BTN_RESET_BREAK = 3; // RX -> reset to break duration

constexpr uint32_t WORK_SECONDS = 25 * 60;
constexpr uint32_t BREAK_SECONDS = 5 * 60;
constexpr uint32_t DEBOUNCE_MS = 30;

// Segment bytes: bit0=A ... bit6=G, bit7=DP.
// shiftOut MSBFIRST lands bit7 on Q7, bit0 on Q0 -> matches Q0=A..Q7=DP wiring.
constexpr uint8_t FONT[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

uint8_t frame[4] = {0, 0, 0, 0}; // raw segment byte per digit

bool running = false;
uint32_t remainingSeconds = WORK_SECONDS;
uint32_t lastTickMs = 0;

struct Button
{
    uint8_t pin;
    bool stableState = HIGH; // idle/released
    bool lastReading = HIGH;
    uint32_t lastChangeMs = 0;
};

Button btnToggle{PIN_BTN_TOGGLE};
Button btnResetWork{PIN_BTN_RESET_WORK};
Button btnResetBreak{PIN_BTN_RESET_BREAK};

// Debounced press-edge detector: returns true once when the pin settles LOW
// after having been stably HIGH.
bool pressed(Button &b)
{
    bool reading = digitalRead(b.pin);
    if (reading != b.lastReading)
    {
        b.lastChangeMs = millis();
        b.lastReading = reading;
    }

    bool firedEdge = false;
    if ((millis() - b.lastChangeMs) > DEBOUNCE_MS && reading != b.stableState)
    {
        bool wasReleased = b.stableState == HIGH;
        b.stableState = reading;
        firedEdge = wasReleased && reading == LOW;
    }
    return firedEdge;
}

void pollButtons()
{
    if (pressed(btnToggle))
    {
        running = !running;
    }
    if (pressed(btnResetWork))
    {
        remainingSeconds = WORK_SECONDS;
        running = false;
    }
    if (pressed(btnResetBreak))
    {
        remainingSeconds = BREAK_SECONDS;
        running = false;
    }
}

void tickCountdown()
{
    if (!running)
        return;
    uint32_t now = millis();
    if (now - lastTickMs < 1000)
        return;
    lastTickMs = now;
    if (remainingSeconds > 0)
        remainingSeconds--;
}

void writeSegments(uint8_t b)
{
    digitalWrite(PIN_LATCH, LOW);
    shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, b);
    digitalWrite(PIN_LATCH, HIGH);
}

void showNumber(uint16_t n)
{
    for (int i = 3; i >= 0; --i)
    {
        frame[i] = FONT[n % 10];
        n /= 10;
    }
}

// Multiplex: one digit per pass, ~2 ms each -> ~125 Hz full-frame refresh.
// Non-blocking so it will coexist with the websocket loop later.
void refresh()
{
    static uint8_t d = 0;
    static uint32_t last = 0;
    if (micros() - last < 2000)
        return;
    last = micros();

    digitalWrite(DIGIT_PINS[d], HIGH); // blank current digit before switching
    d = (d + 1) & 3;                   // (prevents ghosting between digits)
    writeSegments(frame[d]);
    digitalWrite(DIGIT_PINS[d], LOW);
}

void setup()
{
    // TX-only serial keeps GPIO3 (RX) free for the third button
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

    pinMode(PIN_DATA, OUTPUT);
    pinMode(PIN_CLOCK, OUTPUT);
    pinMode(PIN_LATCH, OUTPUT);
    for (uint8_t p : DIGIT_PINS)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH); // all digits off
    }

    pinMode(PIN_BTN_TOGGLE, INPUT_PULLUP);
    pinMode(PIN_BTN_RESET_WORK, INPUT_PULLUP);
    pinMode(PIN_BTN_RESET_BREAK, INPUT_PULLUP);

    // Lamp test: all 8 segments on each digit in turn, DIG1 -> DIG4
    for (uint8_t d = 0; d < 4; ++d)
    {
        writeSegments(0xFF);
        digitalWrite(DIGIT_PINS[d], LOW);
        delay(500);
        digitalWrite(DIGIT_PINS[d], HIGH);
    }
    Serial.println(F("lamp test done, timer ready"));
}

void loop()
{
    pollButtons();
    tickCountdown();

    uint32_t mm = remainingSeconds / 60;
    uint32_t ss = remainingSeconds % 60;
    showNumber(static_cast<uint16_t>(mm * 100 + ss));

    refresh();
}
