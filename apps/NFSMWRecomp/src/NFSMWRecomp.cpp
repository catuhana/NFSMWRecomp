#include "NFSMWRecomp.hpp"

#include "generated/default/NFSMWRecomp_init.h" // IWYU pragma: keep

#include "ISOExtract.hpp"
#include "Patches.hpp"
#include "UI/ISOExtractDialog.hpp"

#include <filesystem>
#include <utility>

// Force the use of discrete GPUs on devices with hybric graphics.
#ifdef _WIN32
#include <windows.h>

extern "C" {
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

namespace NFSMW {

namespace {
/**
 * @brief Populates missing path configurations with default values.
 *
 * If any of the paths are not set (not explicitly set via config file or
 * command line) for things like debugging purposes, have them default to
 * subdirectories of the executable folder.
 */
void SetPaths(rex::PathConfig &paths) {
  const auto executable_folder = rex::filesystem::GetExecutableFolder();

  if (paths.game_data_root.empty()) {
    paths.game_data_root = executable_folder / "game";
  }

  if (!rex::cvar::HasNonDefaultValue("user_data_root")) {
    paths.user_data_root = executable_folder / "userdata";
  }

  if (!rex::cvar::HasNonDefaultValue("cache_root")) {
    paths.cache_root = executable_folder / "cache";
  }
}
} // namespace

auto App::Create(rex::ui::WindowedAppContext &ctx)
    -> std::unique_ptr<rex::ui::WindowedApp> {
  return std::unique_ptr<App>(new App(ctx, "NFSMWRecomp", PPCImageConfig));
}

auto App::OnFinalizePaths(const rex::PathConfig &defaults,
                          std::function<void(rex::PathConfig)> resume)
    -> std::optional<rex::PathConfig> {
  if (std::filesystem::exists(defaults.game_data_root /
                              ISOExtract::kCompleteMarker)) {
    return defaults;
  }

  new UI::ISOExtract::Dialog(imgui_drawer(), defaults.game_data_root,
                             std::move(resume));

  return std::nullopt;
}

void App::OnConfigurePaths(rex::PathConfig &paths) { SetPaths(paths); }

void App::OnPostLoadXexImage() {
  if (auto *memory = runtime()->memory()) {
    Patches::InstallAll<Patches::PostProcessing, Patches::BlackEdition>(
        *memory);
  }
}

} // namespace NFSMW

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
REX_DEFINE_APP(NFSMW, NFSMW::App::Create)
