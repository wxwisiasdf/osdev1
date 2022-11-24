/// tour.cxx
/// Touring program

#include <optional>
#include <kernel/appkit.hxx>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> g_Desktop;

int UDOS_32Main(char32_t[])
{
    TTY::Print("multimedia spieler: Wilkommen");
#if 0
    static std::optional<SoundBlaster16> sb16;
    sb16.emplace(0x20, 2, SoundBlaster16::TransferMode::PLAYSOUND, SoundBlaster16::PCMMode::PCM_8);

    auto& appWin = g_Desktop->AddChild<UI::Window>();
    appWin.SetText("Music player");
    appWin.width = 200;
    appWin.height = 150;
    appWin.Decorate();

    auto& initBtn = appWin.AddChild<UI::Button>();
    initBtn.flex = UI::Widget::Flex::ROW;
    initBtn.width = 128;
    initBtn.height = 12;
    initBtn.SetText("SoundBlaster16");
    initBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool,
                        bool) -> void
                        { });

    static size_t audioOffset = 0;
    sb16->FillBuffer = [](void *data, size_t len) {
        if (audioOffset > theme_raw_len - len)
            audioOffset = 0;
        std::memcpy(data, &theme_raw[audioOffset], len);
        audioOffset += len;
    };
    sb16->sampleRate = 8000;//22050;
    sb16->Start();

    while (!appWin.isClosed)
        Task::Switch();
    appWin.Kill();
#endif
    return 0;
}

__attribute__((section(".text.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
