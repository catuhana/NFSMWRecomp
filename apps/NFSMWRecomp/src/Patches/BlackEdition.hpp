#pragma once

#include <rex/memory.h>

#include <cstdint>

namespace NFSMW::Patches {

struct BlackEdition {
  static constexpr std::uint32_t kFlagVirtualAddress = 0x82A2CE06;

  static void Install(rex::memory::Memory &memory);
  static void Apply(rex::memory::Memory &memory) noexcept;
};

} // namespace NFSMW::Patches
