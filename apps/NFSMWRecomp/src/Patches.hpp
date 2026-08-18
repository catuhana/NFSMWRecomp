#pragma once

#include <rex/memory.h>

#include <concepts>

namespace NFSMW::Patches
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

#include "Patches/PostProcessing.hpp"
