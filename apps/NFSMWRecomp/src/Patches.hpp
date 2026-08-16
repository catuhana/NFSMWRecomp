#pragma once

#include <concepts>

#include <rex/memory.h>

namespace nfsmw::patches
{

  template <typename T>
  concept Patch = requires(rex::memory::Memory &memory) {
    { T::Install(memory) } -> std::same_as<void>;
  };

  template <Patch... TPatches>
  void InstallAll(rex::memory::Memory &memory)
  {
    (TPatches::Install(memory), ...);
  }

}
