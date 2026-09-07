#include "BlackEdition.hpp"

#include <rex/cvar.h>

// NOLINTNEXTLINE
REXCVAR_DEFINE_BOOL(black_edition, true, "Patches",
                    "Enable Black Edition content")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

namespace NFSMW::Patches {

void BlackEdition::Install(rex::memory::Memory &memory) {
  if (REXCVAR_GET(black_edition)) {
    Apply(memory);
  }
}

void BlackEdition::Apply(rex::memory::Memory &memory) noexcept {
  if (auto *flag = memory.TranslateVirtual(kFlagVirtualAddress)) {
    *flag = 1;
  }
}

} // namespace NFSMW::Patches
