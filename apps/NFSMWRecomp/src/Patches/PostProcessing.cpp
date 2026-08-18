#include "PostProcessing.hpp"

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(disable_post_processing,
                    false,
                    "Patches",
                    "Disable post-processing effects");

namespace NFSMW::Patches
{
  void PostProcessing::Install(rex::memory::Memory &memory)
  {
    SetDisabled(memory, REXCVAR_GET(disable_post_processing));

    rex::cvar::RegisterChangeCallback(
        "disable_post_processing",
        [&memory](std::string_view, std::string_view new_value) noexcept
        {
          SetDisabled(memory, new_value == "true");
        });
  }

  void PostProcessing::SetDisabled(rex::memory::Memory &memory, bool disabled) noexcept
  {
    if (auto *flag = reinterpret_cast<std::uint8_t *>(memory.TranslateVirtual(kFlagVirtualAddress)))
    {
      *flag = disabled ? 0 : 1;
    }
  }

}
