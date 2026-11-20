#pragma once
#include "gpio_buttons.h"

namespace bridge {
inline bool remoteWakePin(int pin) {
  if (!buttonPinAllowed(pin)) return false;
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  return pin <= 5;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  return true;
#else
  return pin == 25 || pin == 26 || pin == 27 || pin == 32 || pin == 33;
#endif
}

inline bool validRemoteButtons(const ButtonBinding (&buttons)[2]) {
  return remoteWakePin(buttons[0].pin) && remoteWakePin(buttons[1].pin) &&
         buttons[0].pin != buttons[1].pin && buttons[0].action != buttons[1].action &&
         validButtonBindings(buttons);
}

enum class RemoteGesture { Wait, Sleep, First, Second, Exit };

class RemotePress {
 public:
  RemotePress(unsigned wakeMask, uint32_t now)
    : wakeMask_(wakeMask), started_(now), chord_(wakeMask == 3) {}

  RemoteGesture poll(unsigned pressed, uint32_t now) {
    if (pressed != sample_) {
      sample_ = pressed;
      changed_ = now;
    }
    if (pressed == 3) {
      // Suppress both commands even if the user releases the chord early.
      chord_ = true;
      if (uint32_t(now - changed_) >= 5000) return RemoteGesture::Exit;
    }
    if (pressed || uint32_t(now - changed_) < 40 ||
        uint32_t(now - started_) < 200) return RemoteGesture::Wait;
    if (!chord_ && wakeMask_ == 1) return RemoteGesture::First;
    if (!chord_ && wakeMask_ == 2) return RemoteGesture::Second;
    return RemoteGesture::Sleep;
  }

 private:
  unsigned wakeMask_;
  uint32_t started_, changed_ = 0;
  unsigned sample_ = 0;
  bool chord_;
};
}
