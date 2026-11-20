#include "remote_buttons.h"
#include <cassert>
#include <limits>
using namespace bridge;
int main() {
  ButtonBinding bindings[2]; assert(validButtonBindings(bindings));
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  const int pin=3, second=4; assert(!buttonPinAllowed(9)); assert(!buttonPinAllowed(18));
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  const int pin=4, second=5; assert(!buttonPinAllowed(0)); assert(!buttonPinAllowed(19)); assert(!buttonPinAllowed(33));
#else
  const int pin=25, second=26; assert(!buttonPinAllowed(0)); assert(!buttonPinAllowed(6)); assert(!buttonPinAllowed(34));
#endif
  bindings[0].pin=pin;bindings[0].action=ButtonAction::AllOn;
  bindings[1].pin=second;bindings[1].action=ButtonAction::AllOff;assert(validButtonBindings(bindings));
  assert(validRemoteButtons(bindings));
  bindings[1].action=ButtonAction::AllOn;assert(!validRemoteButtons(bindings));
  bindings[1].action=ButtonAction::AllOff;
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  assert(buttonPinAllowed(7)&&!remoteWakePin(7));
#elif !defined(CONFIG_IDF_TARGET_ESP32S3)
  assert(buttonPinAllowed(19)&&!remoteWakePin(19));
#endif
  bindings[1].pin=pin;assert(!validButtonBindings(bindings));
  bindings[1]=ButtonBinding{};assert(validButtonBindings(bindings));
  bindings[1].pin=second;assert(!validButtonBindings(bindings));
  ButtonDebouncer button;
  // Held at startup cannot fire. Release arms after debounce, not on first edge.
  assert(!button.poll(false,100));assert(!button.poll(false,140));assert(!button.poll(false,900));
  assert(!button.poll(true,901));assert(!button.poll(true,940));assert(!button.poll(true,941));
  assert(!button.poll(false,950));assert(!button.poll(true,960));assert(!button.poll(false,970));
  assert(!button.poll(false,1009));assert(button.poll(false,1010));assert(!button.poll(false,4000));
  assert(!button.poll(true,4001));assert(!button.poll(true,4041));
  assert(!button.poll(false,4050));assert(button.poll(false,4090));
  // An initial high sample is not yet a stable release (including boot bounce).
  ButtonDebouncer bounce;assert(!bounce.poll(true,500));assert(!bounce.poll(false,501));assert(!bounce.poll(false,541));
  assert(!bounce.poll(true,550));assert(!bounce.poll(true,590));assert(!bounce.poll(false,600));assert(bounce.poll(false,640));
  // millis rollover remains bounded.
  ButtonDebouncer wrap;assert(!wrap.poll(true,UINT32_MAX-100));assert(!wrap.poll(true,UINT32_MAX-60));
  assert(!wrap.poll(false,UINT32_MAX-20));assert(!wrap.poll(false,10));assert(wrap.poll(false,20));
  RemotePress cold(0,100);
  assert(cold.poll(0,300)==RemoteGesture::Sleep);
  RemotePress first(1,100);
  assert(first.poll(1,100)==RemoteGesture::Wait);
  assert(first.poll(0,305)==RemoteGesture::Wait);
  assert(first.poll(0,345)==RemoteGesture::First);
  RemotePress secondTap(2,100);
  assert(secondTap.poll(0,300)==RemoteGesture::Second);
  RemotePress abortedChord(1,100);
  assert(abortedChord.poll(3,120)==RemoteGesture::Wait);
  assert(abortedChord.poll(1,500)==RemoteGesture::Wait);
  assert(abortedChord.poll(0,520)==RemoteGesture::Wait);
  assert(abortedChord.poll(0,560)==RemoteGesture::Sleep);
  RemotePress exit(1,UINT32_MAX-99);
  assert(exit.poll(3,UINT32_MAX-99)==RemoteGesture::Wait);
  assert(exit.poll(3,4899)==RemoteGesture::Wait);
  assert(exit.poll(3,4900)==RemoteGesture::Exit);
}
