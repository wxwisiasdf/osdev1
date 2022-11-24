/// run.cxx
/// Program runner

#include <optional>
#include <kernel/appkit.hxx>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> g_Desktop;
int UDOS_32Main(char32_t[])
{
    static std::optional<UI::Window> runWin;
    runWin.emplace();
    runWin->SetText("Run a program");
    runWin->x = runWin->y = 0;
    runWin->width = 200;
    runWin->height = 64;
    runWin->Decorate();
    g_Desktop->AddChildDirect(runWin.value());

    static std::optional<UI::Textbox> filepathTextbox;
    filepathTextbox.emplace();
    filepathTextbox->x = filepathTextbox->y = 0;
    filepathTextbox->width = runWin->width - 12;
    filepathTextbox->height = 12;
    filepathTextbox->SetText("");
    filepathTextbox->SetTooltipText("The filepath for executing the program");
    filepathTextbox->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {

    });
    runWin->AddChildDirect(filepathTextbox.value());

    static std::optional<UI::Button> runBtn;
    runBtn.emplace();
    runBtn->x = 0;
    runBtn->y = 16;
    runBtn->width = runWin->width - 12;
    runBtn->height = 12;
    runBtn->SetText("Run");
    runBtn->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
        Task::Add([]() -> void  {
            static std::optional<UI::Window> errorWin;
            errorWin.emplace();
            errorWin->SetText("Error notFound");
            errorWin->x = errorWin->y = 0;
            errorWin->width = 150;
            errorWin->height = 48;
            errorWin->Decorate();
            g_Desktop->AddChildDirect(errorWin.value());
            while (!errorWin->isClosed)
                Task::Switch();
            errorWin.reset();
        }, nullptr, false);
    });
    runWin->AddChildDirect(runBtn.value());
    while (!runWin->isClosed)
        Task::Switch();
    return 0;
}

__attribute__((section(".text.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
