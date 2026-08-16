#pragma once

#include <rex/cvar.h>
#include <rex/memory.h>

#include <cstdint>
#include <string_view>

REXCVAR_DEFINE_BOOL(disable_post_processing, false, "Patches", "Disable post-processing visual treatments");

namespace nfsmw::patches
{

  struct PostProcessing
  {
    static constexpr uint32_t kFlagVirtualAddress = 0x828F48B2;

    static void Install(rex::memory::Memory &memory)
    {
      SetDisabled(memory, REXCVAR_GET(disable_post_processing));

      rex::cvar::RegisterChangeCallback(
          "disable_post_processing",
          [&memory](std::string_view, std::string_view new_value) noexcept
          {
            SetDisabled(memory, new_value == "true");
          });
    }

    static void SetDisabled(rex::memory::Memory &memory, bool disabled) noexcept
    {
      if (auto *flag = reinterpret_cast<uint8_t *>(memory.TranslateVirtual(kFlagVirtualAddress)))
      {
        *flag = disabled ? 0 : 1;
      }
    }
  };

}
