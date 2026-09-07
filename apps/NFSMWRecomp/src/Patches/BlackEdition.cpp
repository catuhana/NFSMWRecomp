#include "BlackEdition.hpp"

#include <rex/cvar.h>

#include <string_view>

// NOLINTNEXTLINE
REXCVAR_DEFINE_BOOL(black_edition, true, "Patches",
                    "Enable Black Edition content");

namespace NFSMW::Patches {

void BlackEdition::Install(rex::memory::Memory &memory) {
  SetEnabled(memory, REXCVAR_GET(black_edition));

  rex::cvar::RegisterChangeCallback(
      "black_edition",
      [&memory](std::string_view, std::string_view new_value) noexcept {
        SetEnabled(memory, new_value == "true");
      });
}

void BlackEdition::SetEnabled(rex::memory::Memory &memory,
                              bool enabled) noexcept {
  if (auto *flag = memory.TranslateVirtual(kFlagVirtualAddress)) {
    *flag = enabled ? 1 : 0;
  }
}

} // namespace NFSMW::Patches
