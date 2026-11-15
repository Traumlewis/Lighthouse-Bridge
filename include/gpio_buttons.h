#pragma once
#include <stdint.h>
namespace bridge {
enum class ButtonAction : uint8_t { Disabled, AllOn, AllOff };
struct ButtonBinding { int8_t pin = -1; ButtonAction action = ButtonAction::Disabled; };
// Conservative exposed GPIOs: exclude boot straps, flash/PSRAM, USB, UART and LEDs.
inline bool buttonPinAllowed(int pin) {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  return pin == 0 || pin == 1 || (pin >= 3 && pin <= 7) || pin == 10;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  return pin == 1 || pin == 2 || (pin >= 4 && pin <= 18) || pin == 21;
#else
  return pin == 18 || pin == 19 || pin == 21 || pin == 22 || pin == 23 ||
         pin == 25 || pin == 26 || pin == 27 || pin == 32 || pin == 33;
#endif
}
inline bool validButtonBindings(const ButtonBinding (&buttons)[2]) {
  for (const auto& button : buttons) {
    if (button.action == ButtonAction::Disabled) { if (button.pin != -1) return false; }
    else if ((button.action != ButtonAction::AllOn && button.action != ButtonAction::AllOff) ||
             !buttonPinAllowed(button.pin)) return false;
  }
  return buttons[0].pin < 0 || buttons[0].pin != buttons[1].pin;
}
struct ButtonDebouncer {
  bool sample = true, stable = true, armed = false, initialized = false;
  uint32_t changed = 0;
  // Arm only after a stable release; held boot/configuration inputs never fire.
  bool poll(bool released, uint32_t now) {
    if (!initialized) { initialized = true; sample = stable = released; changed = now; return false; }
    if (released != sample) { sample = released; changed = now; }
    if (uint32_t(now - changed) < 40) return false;
    if (sample) { stable = true; armed = true; return false; }
    if (!stable) return false;
    stable = false;
    const bool fire = armed; armed = false; return fire;
  }
};
} // namespace bridge
