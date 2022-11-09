/// tour.cxx
/// Touring program

#include <optional>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> g_Desktop;
int UDOS_32Main(char32_t[])
{
    static std::optional<UI::Window> appWin;
    appWin.emplace();
    appWin->SetText("Music");
    appWin->x = appWin->y = 0;
    appWin->width = 200;
    appWin->height = 150;
    appWin->Decorate();
    g_Desktop->AddChildDirect(appWin.value());

    static std::optional<UI::Button> initBtn;
    initBtn.emplace();
    initBtn->flex = UI::Widget::Flex::ROW;
    initBtn->x = initBtn->y = 0;
    initBtn->width = 128;
    initBtn->height = 12;
    initBtn->SetText("SoundBlaster16");
    initBtn->OnClick = ([](UI::Widget &, unsigned, unsigned, bool,
                        bool) -> void
                        { });
    appWin->AddChildDirect(initBtn.value());
#if 0
    static size_t audioOffset = 0;
    sb16->FillBuffer = [](void *data, size_t len) {
        if (audioOffset > theme_raw_len - len)
            audioOffset = 0;
        std::memcpy(buffer, &theme_raw[audioOffset], len);
        audioOffset += len;
    };
    sb16->sampleRate = 8000;//22050;
    sb16->Start();
#endif
    TTY::Print("multimedia spieler: Wilkommen");
    while (!appWin->isClosed)
        Task::Switch();
    return 0;
}
