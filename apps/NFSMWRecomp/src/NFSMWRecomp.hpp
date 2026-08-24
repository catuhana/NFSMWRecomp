#pragma once

#include <rex/rex_app.h>

#include <functional>
#include <memory>
#include <optional>

namespace NFSMW {

class App : public rex::ReXApp {
public:
  using rex::ReXApp::ReXApp;

  static auto Create(rex::ui::WindowedAppContext &ctx)
      -> std::unique_ptr<rex::ui::WindowedApp>;

protected:
  void OnConfigurePaths(rex::PathConfig &paths) override;
  auto OnFinalizePaths(const rex::PathConfig &defaults,
                       std::function<void(rex::PathConfig)> resume)
      -> std::optional<rex::PathConfig> override;
  void OnPostLoadXexImage() override;
};

} // namespace NFSMW
