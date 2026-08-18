#include "NFSMWRecomp.hpp"

#include <rex/rex_app.h>
#include <rex/cvar.h>

#include "Patches.hpp"
#include "Patches/PostProcessing.cpp"

namespace NFSMW
{

  class App : public rex::ReXApp
  {
  public:
    using rex::ReXApp::ReXApp;

    static std::unique_ptr<rex::ui::WindowedApp> Create(
        rex::ui::WindowedAppContext &ctx)
    {
      return std::unique_ptr<App>(new App(ctx, "NFSMWRecomp",
                                          PPCImageConfig));
    }

  protected:
    void OnPostLoadXexImage() override
    {
      if (auto *memory = runtime()->memory())
      {
        Patches::InstallAll<Patches::PostProcessing>(*memory);
      }
    }
  };

}

REX_DEFINE_APP(NFSMW, NFSMW::App::Create)
