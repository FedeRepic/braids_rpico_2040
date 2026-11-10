#pragma once
#include "ControlLogic.h"

// These execute at compile time against the actual control logic, with no
// firmware runtime cost. They cover physical edge traces and timing boundaries.
namespace control_checks {
template <unsigned N>
constexpr int Decode(const uint8_t (&trace)[N]) {
  controls::RotaryDecoder decoder(trace[0]);
  int steps = 0;
  for (unsigned i = 1; i < N; ++i) steps += decoder.Update(trace[i]);
  return steps;
}
constexpr uint8_t clockwise[] = {3, 1, 0, 2, 3};
constexpr uint8_t reverse[] = {3, 2, 0, 1, 3};
constexpr uint8_t bounce[] = {3, 1, 3, 1, 0, 1, 0, 2, 3};
constexpr uint8_t cancelled[] = {3, 1, 0, 1, 3};
constexpr uint8_t invalid[] = {3, 0, 3};
static_assert(Decode(clockwise) == controls::kEncoderDirection, "Encoder clockwise");
static_assert(Decode(reverse) == -controls::kEncoderDirection, "Encoder reverse");
static_assert(Decode(bounce) == controls::kEncoderDirection, "Encoder bounce");
static_assert(Decode(cancelled) == 0, "Encoder partial reverse");
static_assert(Decode(invalid) == 0, "Encoder invalid transitions");

constexpr bool CheckMenu() {
  controls::Menu menu;
  menu.Turn(-1);
  if (menu.field != controls::Trigger || menu.settings.model != 0) return false;
  menu.Turn(1);
  menu.Press(controls::ButtonEvent::Click);
  menu.Turn(-1);
  if (menu.settings.model != 46 || !menu.editing) return false;
  menu.Turn(1);
  if (menu.settings.model != 0) return false;
  menu.field = controls::Note;
  menu.Turn(-1000);
  if (menu.settings.note != 0) return false;
  menu.Turn(1000);
  if (menu.settings.note != 127) return false;
  menu.field = controls::Level;
  menu.Turn(-1000);
  if (menu.settings.level != 0) return false;
  menu.Turn(1000);
  if (menu.settings.level != 100) return false;
  menu.field = controls::Trigger;
  menu.Turn(-1);
  if (menu.settings.automatic) return false;
  menu.Turn(1);
  if (!menu.settings.automatic) return false;
  menu.Press(controls::ButtonEvent::Hold);
  return menu.strike_sequence == 1 && menu.editing;
}
static_assert(CheckMenu(), "Menu wrapping, parameter bounds and hold");

constexpr bool CheckButton(uint32_t start) {
  using controls::ButtonEvent;
  controls::Button button;
  if (button.Update(true, start) != ButtonEvent::None) return false;
  button.Update(false, start + 5);
  button.Update(true, start + 10);
  button.Update(true, start + 35);
  button.Update(false, start + 100);
  if (button.Update(false, start + 124) != ButtonEvent::None) return false;
  if (button.Update(false, start + 125) != ButtonEvent::Click) return false;
  if (button.Update(false, start + 130) != ButtonEvent::None) return false;
  button.Update(true, start + 200);
  button.Update(true, start + 225);
  if (button.Update(true, start + 824) != ButtonEvent::None) return false;
  if (button.Update(true, start + 825) != ButtonEvent::Hold) return false;
  if (button.Update(true, start + 900) != ButtonEvent::None) return false;
  button.Update(false, start + 950);
  return button.Update(false, start + 975) == ButtonEvent::None;
}
static_assert(CheckButton(0), "Button bounce, click, single hold and no hold-click");
static_assert(CheckButton(0xFFFFFF00u), "Button timer rollover");
}  // namespace control_checks
