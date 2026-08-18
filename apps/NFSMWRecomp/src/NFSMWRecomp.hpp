#pragma once

#include <rex/rex_app.h>

#include <memory>

namespace rex::ui
{
  class WindowedApp;
  class WindowedAppContext;
}

namespace NFSMW
{

  class App : public rex::ReXApp
  {
  public:
    using rex::ReXApp::ReXApp;

    static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext &ctx);

  protected:
    void OnPostLoadXexImage() override;
  };

}
