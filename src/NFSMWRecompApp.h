#pragma once

#include <rex/rex_app.h>

class NFSMWRecompApp : public rex::ReXApp
{
public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext &ctx)
  {
    return std::unique_ptr<NFSMWRecompApp>(new NFSMWRecompApp(ctx, "NFSMWRecomp",
                                                              PPCImageConfig));
  }
};
