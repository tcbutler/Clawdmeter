#include "../../hal/power_hal.h"
#include "board.h"
#include "../../idle.h"

#include <Arduino.h>

// No PMU, no battery, no PWR button — this board is USB-powered and has a
// single user button (BOOT). That button is wired here rather than to
// input_hal so the screen/brightness cycling and the hold-to-pair gesture stay
// reachable on a board whose resistive touch may need calibrating first.
//
// The three edges the shared loop consumes (short press, long-press crossing,
// release) are derived here from one debounced GPIO.

static const uint32_t DEBOUNCE_MS = 25;

static bool     down = false;
static bool     long_sent = false;
static uint32_t down_since_ms = 0;
static uint32_t last_edge_ms = 0;

static bool pressed_flag = false;
static bool long_pressed_flag = false;
static bool released_flag = false;

// --- Ambient light -----------------------------------------------------------
// Read the LDR divider and tell idle when the room has gone dark, so the panel
// blanks itself overnight instead of glowing at an empty desk. Hysteresis here
// covers electrical flicker; the *timing* judgement (how long dark counts, and
// staying awake after a wake) belongs to idle, not to a board.
static bool     ldr_dark = false;
static uint32_t ldr_last_ms = 0;

static void ambient_tick(uint32_t now) {
    if (now - ldr_last_ms < LDR_SAMPLE_MS) return;
    ldr_last_ms = now;

    // A single analogRead here is unusable: the divider is high-impedance when
    // dark, so the ADC's sample-and-hold never settles and consecutive reads
    // swing across most of the range (measured: raw=1200 next to mV=384 on the
    // same sample). Median-of-N throws away those outliers, and an EMA on top
    // gives the decision a ~2.5 s time constant — slow enough that noise can't
    // flip the state, fast enough that a hand over the sensor still registers.
    int samples[LDR_OVERSAMPLE];
    for (int i = 0; i < LDR_OVERSAMPLE; ++i) {
        samples[i] = analogRead(LDR_ADC_PIN);
        delayMicroseconds(200);          // let the S/H recover between reads
    }
    for (int i = 1; i < LDR_OVERSAMPLE; ++i) {      // insertion sort, N is tiny
        const int v = samples[i];
        int j = i - 1;
        while (j >= 0 && samples[j] > v) { samples[j + 1] = samples[j]; --j; }
        samples[j + 1] = v;
    }
    const int median = samples[LDR_OVERSAMPLE / 2];

    static int smoothed = -1;
    smoothed = (smoothed < 0) ? median
                              : smoothed + (median - smoothed) / LDR_EMA_DIVISOR;
    const int raw = smoothed;
    // Work in "darkness units" so the thresholds mean the same thing whichever
    // leg of the divider the LDR sits in.
#if LDR_DARK_ABOVE
    const int dark_metric = raw;
#else
    const int dark_metric = 4095 - raw;
#endif
    // Schmitt trigger: cross the far threshold to change state, so a reading
    // hovering on the boundary doesn't flap the panel on and off.
    if (!ldr_dark && dark_metric >= LDR_DARK_LEVEL)      ldr_dark = true;
    else if (ldr_dark && dark_metric <= LDR_LIGHT_LEVEL) ldr_dark = false;

    idle_set_ambient_dark(ldr_dark);

#ifdef LDR_DEBUG
    // Track the extremes so a capture needn't be timed perfectly: the span
    // between them is what actually decides a usable threshold.
    static int lo = 4095, hi = 0;
    static uint32_t last_log = 0;
    if (raw < lo) lo = raw;
    if (raw > hi) hi = raw;
    if (now - last_log >= 2000) {
        last_log = now;
        Serial.printf("ldr med=%4d smooth=%4d  seen[min=%4d max=%4d]  dark=%d\n",
                      median, raw, lo, hi, ldr_dark ? 1 : 0);
    }
#endif
}

void power_hal_init(void) {
    pinMode(BTN_PWR_GPIO, INPUT_PULLUP);
    down = false;
    last_edge_ms = millis();
}

void power_hal_tick(void) {
    const uint32_t now = millis();
    ambient_tick(now);
    const bool raw = digitalRead(BTN_PWR_GPIO) == LOW;

    if (raw != down) {
        if (now - last_edge_ms < DEBOUNCE_MS) return;
        last_edge_ms = now;
        down = raw;
        if (down) {
            long_sent = false;
            down_since_ms = now;
        } else {
            // A press that already fired its long-press edge must not also
            // fire a short press — the shared pair gesture is long + release.
            if (!long_sent) pressed_flag = true;
            released_flag = true;
        }
        return;
    }

    if (down && !long_sent && (now - down_since_ms) >= BTN_PWR_LONG_MS) {
        long_sent = true;
        long_pressed_flag = true;
    }
}

int  power_hal_battery_pct(void) { return -1; }     // no battery circuit
bool power_hal_is_charging(void) { return false; }
bool power_hal_is_vbus_in(void)  { return true; }   // USB is the only supply

bool power_hal_pwr_pressed(void) {
    const bool v = pressed_flag;
    pressed_flag = false;
    return v;
}

bool power_hal_pwr_long_pressed(void) {
    const bool v = long_pressed_flag;
    long_pressed_flag = false;
    return v;
}

bool power_hal_pwr_released(void) {
    const bool v = released_flag;
    released_flag = false;
    return v;
}
