/// system.cxx
/// System settings program

#include <optional>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> desktop;
int UDOS_32Main(char32_t[])
{
    static std::optional<UI::Window> systemWin;
    systemWin.emplace();
    systemWin->SetText("System information");
    systemWin->x = 0;
    systemWin->y = 0;
    systemWin->width = 200;
    systemWin->height = 150;
    systemWin->Decorate();
    desktop->AddChild(systemWin.value());

    static std::optional<UI::Textbox> infoTextbox;
    infoTextbox.emplace();
    infoTextbox->SetText("...");
    infoTextbox->y = infoTextbox->x = 0;
    infoTextbox->width = systemWin->width - 24;
    infoTextbox->height = systemWin->height - 32;
    infoTextbox->OnUpdate = [](UI::Widget& w) {
        auto& o = static_cast<UI::Textbox&>(w);
        auto fmtPrint = [](const char *fmt, ...) {
            va_list args;
            va_start(args, fmt);
            static char tmpbuf[300] = {};
            TTY::Print_1(tmpbuf, sizeof(tmpbuf), fmt, args);
            va_end(args);
            return tmpbuf;
        };
        auto ts = Task::GetSummary();
        o.SetText(fmtPrint("A-Tasks: %u\nT-Tasks: %u", ts.nActive, ts.nTotal));
    };
    infoTextbox->OnUpdate(infoTextbox.value());
    systemWin->AddChild(infoTextbox.value());

    while (!systemWin->isClosed)
        Task::Switch();
    return 0;
}
