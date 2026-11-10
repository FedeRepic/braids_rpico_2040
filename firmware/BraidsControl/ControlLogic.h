#pragma once
#include <stdint.h>

namespace controls {
constexpr uint8_t kModelCount = 47;
constexpr uint8_t kFieldCount = 6;
constexpr int8_t kEdgesPerDetent = 4;  // Typical CLK/DT/SW (KY-040) breakout.
constexpr int kEncoderDirection = 1;  // Set to -1 to reverse knob direction.
enum Field : uint8_t { Model, Note, Timbre, Color, Level, Trigger };
enum class ButtonEvent { None, Click, Hold };

struct Settings {
  uint8_t model = 0;
  uint8_t note = 60;
  uint8_t timbre = 50;
  uint8_t color = 25;
  uint8_t level = 25;
  bool automatic = true;
};

constexpr int Clamp(int value, int minimum, int maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
}

constexpr int Wrap(int value, int count) {
  return ((value % count) + count) % count;
}

// Invalid two-bit jumps are discarded; reversed/bouncing edges cancel out.
class RotaryDecoder {
 public:
  constexpr explicit RotaryDecoder(uint8_t initial = 3) : previous_(initial) {}
  constexpr int Update(uint8_t state) {
    constexpr int8_t transitions[16] = {
        0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    const uint8_t changed = previous_ ^ state;
    if (changed == 3) {
      accumulated_ = 0;
    } else {
      accumulated_ += transitions[(previous_ << 2) | state];
    }
    previous_ = state;
    if (accumulated_ >= kEdgesPerDetent) {
      accumulated_ -= kEdgesPerDetent;
      return kEncoderDirection;
    }
    if (accumulated_ <= -kEdgesPerDetent) {
      accumulated_ += kEdgesPerDetent;
      return -kEncoderDirection;
    }
    return 0;
  }
 private:
  uint8_t previous_;
  int8_t accumulated_ = 0;
};

class Button {
 public:
  constexpr ButtonEvent Update(bool pressed, uint32_t now) {
    if (pressed != raw_) {
      raw_ = pressed;
      changed_ms_ = now;
    }
    if (raw_ != stable_ && uint32_t(now - changed_ms_) >= 25) {
      stable_ = raw_;
      if (stable_) {
        pressed_ms_ = now;
        hold_sent_ = false;
      } else if (!hold_sent_) {
        return ButtonEvent::Click;
      }
    }
    if (stable_ && raw_ && !hold_sent_ && uint32_t(now - pressed_ms_) >= 600) {
      hold_sent_ = true;
      return ButtonEvent::Hold;
    }
    return ButtonEvent::None;
  }
 private:
  bool raw_ = false;
  bool stable_ = false;
  bool hold_sent_ = false;
  uint32_t changed_ms_ = 0;
  uint32_t pressed_ms_ = 0;
};

class Menu {
 public:
  Settings settings;
  uint8_t field = Model;
  bool editing = false;
  uint32_t strike_sequence = 0;

  constexpr void Turn(int steps) {
    if (!editing) {
      field = static_cast<uint8_t>(Wrap(field + steps, kFieldCount));
      return;
    }
    switch (field) {
      case Model: settings.model = Wrap(settings.model + steps, kModelCount); break;
      case Note: settings.note = Clamp(settings.note + steps, 0, 127); break;
      case Timbre: settings.timbre = Clamp(settings.timbre + steps, 0, 100); break;
      case Color: settings.color = Clamp(settings.color + steps, 0, 100); break;
      case Level: settings.level = Clamp(settings.level + steps, 0, 100); break;
      case Trigger: if (steps != 0) settings.automatic = steps > 0; break;
    }
  }

  constexpr void Press(ButtonEvent event) {
    if (event == ButtonEvent::Click) editing = !editing;
    if (event == ButtonEvent::Hold) ++strike_sequence;
  }
};
}  // namespace controls
