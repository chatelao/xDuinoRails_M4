#include <MfxDecoder.h>

MfxDecoder decoder;
const int PIN_INPUT = 2; // Interrupt pin

volatile uint32_t lastTime = 0;
// Note: Atomic access needed for shared variables on 8-bit AVR
// Using a simple volatile flag and single buffer for demo.
// For robust decoding, use a circular buffer in ISR.
volatile bool hasNewEdge = false;
volatile uint16_t edgeDelta = 0;

void isr() {
    uint32_t now = micros();
    uint32_t delta = now - lastTime;
    if (delta > 65535) delta = 65535;

    edgeDelta = (uint16_t)delta;
    lastTime = now;
    hasNewEdge = true;
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_INPUT, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_INPUT), isr, CHANGE);
    Serial.println("MfxDecoder SimpleDecode Example");

    // Initialize timestamp
    lastTime = micros();
}

void loop() {
    if (hasNewEdge) {
        // Disable interrupts briefly to read safely (on AVR)
        noInterrupts();
        uint16_t dt = edgeDelta;
        hasNewEdge = false;
        interrupts();

        decoder.feedEdge(dt);
    }

    while (decoder.available()) {
        MfxDecoder::DecodedEvent e = decoder.read();
        switch (e.type) {
            case MfxDecoder::EVENT_BIT:
                Serial.print(e.value);
                break;
            case MfxDecoder::EVENT_SYNC:
                Serial.println("\nSYNC");
                break;
            case MfxDecoder::EVENT_ERROR:
                Serial.print("E");
                break;
            default:
                break;
        }
    }
}
