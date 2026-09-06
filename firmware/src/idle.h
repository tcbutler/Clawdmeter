#pragma once
#include <stdbool.h>
#include <stdint.h>   // uint8_t — the header must stand alone

void idle_init(void);
void idle_tick(void);
void idle_note_activity(void);

// Set the "awake" brightness target (0..255). idle owns display brightness
// (it fades between this and 0), so user brightness control routes through
// here. Applied immediately if the screen is currently fully awake; otherwise
// picked up by the next fade-in. See brightness.{h,cpp}.
void idle_set_awake_brightness(uint8_t level);

// Returns true if this press was consumed as a wake-up (caller MUST skip the
// button's normal action). Returns false when already awake — also notes the
// activity, so callers don't need a separate idle_note_activity() call.
bool idle_consume_wake_press(void);

// Touch should NOT count as activity (avoids accidental wakes from pets,
// sleeves, etc.). Callers use this to silently drop touch events while the
// panel is dark.
bool idle_is_asleep(void);

// Report ambient darkness from a board that has a light sensor. Pushed rather
// than pulled: only one board carries an LDR, so a whole HAL header plus seven
// stubs would be more machinery than the feature is worth. Boards without a
// sensor simply never call this and behave exactly as before.
//
// Darkness sleeps the panel even on USB power (which otherwise pins it awake —
// see IDLE_SLEEP_WHEN_CHARGING), after IDLE_DARK_DWELL_MS so a hand passing
// over the sensor doesn't blank the screen. A wake keeps the panel up for
// IDLE_DARK_WAKE_GRACE_MS regardless of darkness, or touching a dark panel
// would light it just long enough to fade straight back out.
void idle_set_ambient_dark(bool dark);
