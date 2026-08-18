#pragma once

#include <rex/memory.h>

#include <cstdint>

namespace NFSMW::Patches
{

  struct PostProcessing
  {
    static constexpr std::uint32_t kFlagVirtualAddress = 0x828F48B2;

    static void Install(rex::memory::Memory &memory);
    static void SetDisabled(rex::memory::Memory &memory, bool disabled) noexcept;
  };

}
