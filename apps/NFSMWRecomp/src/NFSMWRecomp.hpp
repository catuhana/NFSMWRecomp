#pragma once

#include <rex/rex_app.h>

#include <functional>
#include <optional>
#include <memory>

namespace NFSMW
{

  class App : public rex::ReXApp
  {
  public:
    using rex::ReXApp::ReXApp;

    static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext &ctx);

  protected:
    void OnConfigurePaths(rex::PathConfig &paths) override;
    std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig &defaults,
                                                   std::function<void(rex::PathConfig)> resume) override;
    void OnPostLoadXexImage() override;
  };

}
