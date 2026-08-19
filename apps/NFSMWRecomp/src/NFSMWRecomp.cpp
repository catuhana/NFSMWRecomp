#include "NFSMWRecomp.hpp"

#include "generated/default/NFSMWRecomp_init.h"

#include "Patches.hpp"
#include "UI/ISOExtractDialog.hpp"

namespace NFSMW
{

  namespace
  {
    /**
     * @brief Populates missing path configurations with default values.
     *
     * If any of the paths are not set (not explicitly set via config file or command line)
     * for things like debugging purposes, have them default to subdirectories of the executable folder.
     */
    void SetPaths(rex::PathConfig &paths)
    {
      const auto executable_folder = rex::filesystem::GetExecutableFolder();

      if (paths.game_data_root.empty())
      {
        paths.game_data_root = executable_folder / "game";
      }

      if (!rex::cvar::HasNonDefaultValue("user_data_root"))
      {
        paths.user_data_root = executable_folder / "userdata";
      }

      if (!rex::cvar::HasNonDefaultValue("cache_root"))
      {
        paths.cache_root = executable_folder / "cache";
      }
    }
  }

  std::unique_ptr<rex::ui::WindowedApp> App::Create(rex::ui::WindowedAppContext &ctx)
  {
    return std::unique_ptr<App>(new App(ctx, "NFSMWRecomp", PPCImageConfig));
  }

  std::optional<rex::PathConfig> App::OnFinalizePaths(const rex::PathConfig &defaults,
                                                      std::function<void(rex::PathConfig)> resume)
  {
    if (std::filesystem::exists(defaults.game_data_root / UI::kCompleteMarker))
      return defaults;

    new UI::ISOExtractDialog(imgui_drawer(), defaults.game_data_root, std::move(resume));

    return std::nullopt;
  }

  void App::OnConfigurePaths(rex::PathConfig &paths)
  {
    SetPaths(paths);
  }

  void App::OnPostLoadXexImage()
  {
    if (auto *memory = runtime()->memory())
    {
      Patches::InstallAll<Patches::PostProcessing>(*memory);
    }
  }

}

REX_DEFINE_APP(NFSMW, NFSMW::App::Create)
