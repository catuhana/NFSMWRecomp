#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>

#include "Patches.hpp"
#include "Patches/PostProcessing.hpp"

namespace nfsmw
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
				patches::InstallAll<patches::PostProcessing>(*memory);
			}
		}
	};

}
