/*
 * Auris pointer — button-only bring-up test.
 *
 * WIRING — bare 4-leg momentary switch (the intended case)
 *
 *   ESP32 D18 (bottom row, between D5 and D19) -- switch leg A
 *   ESP32 GND (bottom row, next to 3V3)       -- switch leg B, on the OPPOSITE
 *                                                side of the switch
 *   switch legs C and D   --  unused
 *   No VCC anywhere on the switch.
 *
 *   GPIO18 is configured INPUT_PULLUP, so the pin idles HIGH (btn=0) and a press
 *   closes it to GND (btn=1). The internal pull-up replaces the VCC connection: a
 *   bare switch is a contact, not a powered device.
 *
 * WIRING — 3-pin button module labelled VCC / GND / OUT (if that is what you have)
 *
 *   module VCC  --  3V3 (bottom row, next to GND)
 *   module GND  --  GND (bottom row, next to 3V3)
 *   module OUT  --  D18 (GPIO18, bottom row between D5 and D19)
 *
 *   Most such modules present an active-low open-drain output, so the internal
 *   pull-up is harmless and a press still reads as btn=1.
 *
 * DO NOT connect VCC and GND directly to two legs of a bare switch. On a
 * standard 4-leg tact switch the two legs on one side are internally joined;
 * the other pair is joined too, and pressing bridges the pairs. VCC on one leg
 * and GND on any other is therefore a short of the 3.3 V rail while pressed —
 * or permanently, if both are on the same side. If "VCC, GND, signal" seemed to
 * work, it was either a module with its own pull-up or a leg pairing that was
 * not what it looked like. Use the two-wire arrangement above.
 *
 * Purpose: prove one switch on one pin, with no Wi-Fi, no ToF and no IMU in the
 *          way. It samples at 1 kHz, debounces for 20 ms, and prints one column —
 *          `btn`, 1 while the button is pressed and 0 while it is not. Nothing
 *          else is reported: no timing, no counters, no statistics. The onboard
 *          LED mirrors the same debounced state.
 *
 * Output:  a `btn` header followed by one value per line, 10 times a second, so
 *          the column can be watched while the switch is pressed and released.
 *          The header repeats every five seconds for a monitor that attaches late.
 *
 * Build:   arduino-cli compile --profile button-test
 * Flash:   arduino-cli upload  --profile button-test --port /dev/ttyUSB0
 * Monitor: arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
 */

#include <Arduino.h>

/** Button input: pointer bottom-row label D18 (between D5 and D19), same pin the deployed pointer uses. */
static constexpr uint8_t PIN_BUTTON = 18;
/** Onboard LED; mirrors the debounced button state. */
static constexpr uint8_t PIN_LED = 2;

/** Debounce interval in milliseconds; a raw level must persist this long to be accepted. */
static constexpr uint32_t DEBOUNCE_MS = 20;
/** Sampling period in microseconds (1 kHz, fast enough to see chatter). */
static constexpr uint32_t SAMPLE_PERIOD_US = 1000;
/** Column print period in milliseconds (10 Hz: watchable, and still shows liveness). */
static constexpr uint32_t PRINT_PERIOD_MS = 100;
/** Header repeat interval in milliseconds. */
static constexpr uint32_t HEADER_PERIOD_MS = 5000;

/** Last raw pin interpretation: true when the pin reads LOW (pressed). */
static bool g_raw = false;
/** Debounced state: true when the button is accepted as pressed; this is the printed column. */
static bool g_stable = false;
/** Time of the last raw change, in milliseconds; starts the debounce window. */
static uint32_t g_rawChangedMs = 0;
/** Last sample time, in microseconds. */
static uint32_t g_lastSampleUs = 0;
/** Last column print, in milliseconds. */
static uint32_t g_lastPrintMs = 0;
/** Last header print, in milliseconds. */
static uint32_t g_lastHeaderMs = 0;

/**
 * Drive the onboard LED.
 *
 * @param on True to light the LED, false to extinguish it.
 */
static void setLed(bool on) {
  digitalWrite(PIN_LED, on ? HIGH : LOW);
}

/**
 * Print the column header, with the same width as the values below it.
 */
static void printHeader() {
  Serial.printf("%-3s\n", "btn");
}

/**
 * Print the single column: 1 while pressed, 0 while released.
 */
static void printColumn() {
  Serial.printf("%-3d\n", g_stable ? 1 : 0);
}

/**
 * Print the wiring reminder and the boot-time input state.
 *
 * A pressed reading at boot means the pin is already low: either the switch is
 * held, its legs are on the same side of the contact, or something is shorting
 * GPIO18 to ground. The test keeps running so the behaviour can still be
 * observed in the column.
 */
static void printBanner() {
  Serial.println();
  Serial.println("# Auris button-only test");
  Serial.println("# bare switch: GPIO18 -> leg A, GND -> opposite leg B, no VCC");
  Serial.println("# 3-pin module: VCC -> 3V3, GND -> GND, OUT -> GPIO18");
  Serial.printf("# GPIO%u INPUT_PULLUP, debounce %lu ms\n",
                PIN_BUTTON, static_cast<unsigned long>(DEBOUNCE_MS));
  Serial.printf("# boot state: btn=%d\n", g_raw ? 1 : 0);
  if (g_raw) {
    Serial.println("# WARNING: input is already low at boot. If the button is"
                   " untouched, check the leg pairing or a short.");
    Serial.println("# A bare switch must not have VCC on any leg.");
  }
  Serial.println("# btn=1 while pressed, btn=0 while released");
  printHeader();
}

/**
 * Sample the pin and run the debounce state machine.
 *
 * A raw change restarts the debounce window; the state is accepted only once the
 * raw level has been stable for DEBOUNCE_MS. Only the accepted state is printed,
 * so contact chatter inside the window never reaches the column.
 *
 * @param nowMs Current `millis()` value.
 */
static void sampleButton(uint32_t nowMs) {
  const bool raw = digitalRead(PIN_BUTTON) == LOW;
  if (raw != g_raw) {
    g_raw = raw;
    g_rawChangedMs = nowMs;
  }

  if (g_raw == g_stable || nowMs - g_rawChangedMs < DEBOUNCE_MS) {
    return;
  }

  g_stable = g_raw;
}

/** Configure the pin and LED, read the initial state, and print the banner. */
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  setLed(false);

  g_raw = digitalRead(PIN_BUTTON) == LOW;
  g_stable = false;  // force the debounce to accept the boot state after DEBOUNCE_MS
  g_rawChangedMs = millis();

  printBanner();
  g_lastPrintMs = millis();
  g_lastHeaderMs = millis();
  g_lastSampleUs = micros();
  Serial.flush();
}

/**
 * Superloop: sample at 1 kHz, mirror the state on the LED, print the column at 10 Hz.
 *
 * Nothing here blocks on USB or on any peripheral.
 */
void loop() {
  const uint32_t nowUs = micros();
  if (nowUs - g_lastSampleUs >= SAMPLE_PERIOD_US) {
    g_lastSampleUs = nowUs;
    sampleButton(millis());
  }

  setLed(g_stable);

  const uint32_t nowMs = millis();
  if (nowMs - g_lastPrintMs >= PRINT_PERIOD_MS) {
    g_lastPrintMs = nowMs;
    if (nowMs - g_lastHeaderMs >= HEADER_PERIOD_MS) {
      g_lastHeaderMs = nowMs;
      printHeader();
    }
    printColumn();
  }
}
