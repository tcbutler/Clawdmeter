#pragma once

// Auto-sleep / idle screen-off configuration.
// All tunables live here so nothing is hard-coded in main.cpp / idle.cpp.

#define IDLE_TIMEOUT_MS             (30UL * 60UL * 1000UL)  // 30 min
#define IDLE_FADE_OUT_MS            400      // fade-to-black duration
#define IDLE_FADE_IN_MS             180      // wake fade-in (snappier)
#define IDLE_FADE_STEP_MS           20       // tick interval per fade step

#define DISPLAY_DEFAULT_BRIGHTNESS  200      // active-screen brightness

// When false, the device never enters sleep while USB power is present (also
// wakes from sleep when USB is plugged back in). Useful when sitting on a
// desk plugged in — also covers battery-less hardware that's always on USB.
// Set true to sleep regardless of power source.
#define IDLE_SLEEP_WHEN_CHARGING    false

// --- Ambient-light sleep (boards with an LDR; see idle_set_ambient_dark) ---
// How long it must stay dark before the panel fades out. Long enough that a
// hand over the sensor, or someone walking past a lamp, doesn't blank it.
#define IDLE_DARK_DWELL_MS          (20UL * 1000UL)   // 20 s
// After any wake, ignore darkness for this long — otherwise a touch in a dark
// room fades up and immediately back down.
#define IDLE_DARK_WAKE_GRACE_MS     (60UL * 1000UL)   // 60 s

// When true, a touch on the dark panel wakes the device (first touch is
// consumed for wake only, second touch acts normally). When false, touch is
// fully ignored during sleep — useful if cats/sleeves brushing the panel
// overnight would be a problem.
#define IDLE_WAKE_ON_TOUCH          true
