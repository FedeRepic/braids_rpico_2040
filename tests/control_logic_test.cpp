#include "../firmware/BraidsControl/ControlChecks.h"

// All checks run during compilation, so no host compiler/runtime or board is
// needed. Compile with the project's ARM C++ compiler using -std=c++14 -c.
constexpr bool CheckLongTraces() {
  // A whole rotation cycle must decode consistently from any initial phase.
  const uint8_t cycle[] = {3, 1, 0, 2};
  for (int start = 0; start < 4; ++start) {
    controls::RotaryDecoder decoder(cycle[start]);
    int detents = 0;
    for (int edge = 1; edge <= 400; ++edge) {
      detents += decoder.Update(cycle[(start + edge) % 4]);
    }
    if (detents != 100 * controls::kEncoderDirection) return false;
    for (int edge = 1; edge <= 400; ++edge) {
      detents += decoder.Update(cycle[(start + 400 - edge) % 4]);
    }
    if (detents != 0) return false;
  }
  return true;
}

constexpr bool CheckAllFields() {
  // Exercise every menu field, including lower/upper limits and navigation.
  controls::Menu menu;
  for (int field = 0; field < controls::kFieldCount; ++field) {
    if (menu.field != field) return false;
    menu.Press(controls::ButtonEvent::Click);
    menu.Turn(-1000);
    if (field == controls::Note && menu.settings.note != 0) return false;
    if (field == controls::Timbre && menu.settings.timbre != 0) return false;
    if (field == controls::Color && menu.settings.color != 0) return false;
    if (field == controls::Level && menu.settings.level != 0) return false;
    if (field == controls::Trigger && menu.settings.automatic) return false;
    menu.Turn(2000);
    if (menu.settings.model >= controls::kModelCount) return false;
    if (field == controls::Note && menu.settings.note != 127) return false;
    if (field == controls::Timbre && menu.settings.timbre != 100) return false;
    if (field == controls::Color && menu.settings.color != 100) return false;
    if (field == controls::Level && menu.settings.level != 100) return false;
    if (field == controls::Trigger && !menu.settings.automatic) return false;
    menu.Press(controls::ButtonEvent::Click);
    menu.Turn(1);
  }
  return menu.field == 0;
}

constexpr bool CheckButtonWorkflow() {
  // A short contact glitch never toggles editing or produces a Strike.
  controls::Button button;
  controls::Menu button_menu;
  for (uint32_t now = 0; now <= 100; ++now) {
    button_menu.Press(button.Update(now >= 10 && now < 20, now));
  }
  if (button_menu.editing || button_menu.strike_sequence != 0) return false;
  // A valid click enters editing once, then a hold triggers without exiting it.
  for (uint32_t now = 101; now <= 1000; ++now) {
    const bool pressed = (now >= 110 && now < 200) || (now >= 300 && now < 950);
    button_menu.Press(button.Update(pressed, now));
  }
  return button_menu.editing && button_menu.strike_sequence == 1;
}

static_assert(CheckLongTraces(), "Long rotation traces from all four phases");
static_assert(CheckAllFields(), "Every field respects bounds and navigation");
static_assert(CheckButtonWorkflow(), "A real click/hold workflow and short glitch");
