#include "NFSMWRecomp.hpp"

#include "generated/default/NFSMWRecomp_init.h"

#include "Patches.hpp"

namespace NFSMW
{

  std::unique_ptr<rex::ui::WindowedApp> App::Create(rex::ui::WindowedAppContext &ctx)
  {
    return std::unique_ptr<App>(new App(ctx, "NFSMWRecomp", PPCImageConfig));
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
