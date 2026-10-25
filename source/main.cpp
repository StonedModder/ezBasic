#include <switch.h>
#include <pu/Plutonium>
#include <EzBasicApp.hpp>
#include <RomfsBootstrap.hpp>
#include <Utils.hpp>

int main(int argc, char** argv) {
    Result rc = romfsInit();
    if(R_FAILED(rc)) {
        return EXIT_FAILURE;
    }

    auto textOpt = ezbasic::LoadRomfsText("romfs:/bootstrap.bas");
    auto fileOpt = ezbasic::LoadRomfsFile("romfs:/bootstrap.bas");
    if(!textOpt || !fileOpt) {
        romfsExit();
        return EXIT_FAILURE;
    }

    auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
    renderer_opts.UseImage(pu::ui::render::ImgAllFlags);
    renderer_opts.SetPlServiceType(PlServiceType_User);
    renderer_opts.AddDefaultAllSharedFonts();
    renderer_opts.AddExtraDefaultFontSize(56);
    renderer_opts.SetInputPlayerCount(1);
    renderer_opts.AddInputNpadStyleTag(HidNpadStyleSet_NpadStandard);
    renderer_opts.AddInputNpadIdType(HidNpadIdType_Handheld);
    renderer_opts.AddInputNpadIdType(HidNpadIdType_No1);

    auto renderer = pu::ui::render::Renderer::New(renderer_opts);
    auto app = EzBasicApp::New(renderer);

    app->SetBootstrap(*textOpt, fileOpt->sha256, fileOpt->size);

    const auto load_rc = app->Load();
    if(R_FAILED(load_rc)) {
        diagAbortWithResult(load_rc);
    }

    app->ShowWithFadeIn();

    romfsExit();
    return 0;
}
