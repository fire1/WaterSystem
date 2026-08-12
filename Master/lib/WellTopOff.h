#ifndef WellTopOff_h
#define WellTopOff_h

#include <stdint.h>

namespace wellTopOff {

constexpr uint8_t EXTRA_RUNS = 4;

struct State {
  uint8_t remaining = 0;
  bool latched = false;

  void observeFull(uint8_t level, uint8_t fullLevel) {
    if (!latched && level <= fullLevel) {
      latched = true;
      remaining = EXTRA_RUNS;
    }
  }

  bool allowsRun() const { return remaining > 0; }

  void completeRun() {
    if (remaining > 0)
      --remaining;
  }

  void resetOnMainStart() {
    remaining = 0;
    latched = false;
  }
};

inline State state;

} // namespace wellTopOff

#endif
