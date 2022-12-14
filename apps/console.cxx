/// console.cxx
/// Console GUI program

#include <optional>
#include <kernel/appkit.hxx>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> g_Desktop;
int UDOS_32Main(char32_t[])
{
    static std::optional<UI::Window> consoleWin;
    consoleWin.emplace();
    consoleWin->SetText("Kernal console");
    consoleWin->x = consoleWin->y = 0;
    consoleWin->width = 250;
    consoleWin->height = 150;
    consoleWin->Decorate();
    g_Desktop->AddChildDirect(consoleWin.value());

    static std::optional<UI::Terminal> term;
    term.emplace();
    term->x = term->y = 0;
    term->width = consoleWin->width - 8;
    term->height = consoleWin->height - 8 - 24;
    consoleWin->AddChildDirect(term.value());

    static std::optional<UI::Button> attachToKernelBtn;
    static bool isAttachToKernel = false;
    attachToKernelBtn.emplace();
    attachToKernelBtn->SetText("Kernel");
    attachToKernelBtn->x = 64;
    attachToKernelBtn->y = -16;
    attachToKernelBtn->width = 128;
    attachToKernelBtn->height = 12;
    attachToKernelBtn->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
        if(isAttachToKernel) {
            attachToKernelBtn->SetText("Normal");
            attachToKernelBtn->SetTooltipText("Set terminal to \"Normal\" mode, allowing for bidirectional inputs and outputs");
            term->term.AttachToKernel(term->term);
        } else {
            attachToKernelBtn->SetText("Kernel");
            attachToKernelBtn->SetTooltipText("Set terminal to \"Kernel\" mode, allowing for reading the kernel console directly");
            term->term.DetachFromKernel(term->term);
        }
        isAttachToKernel = !isAttachToKernel;
    });
    consoleWin->AddChildDirect(attachToKernelBtn.value());

    while (!consoleWin->isClosed)
        Task::Switch();
    consoleWin->Kill();
    return 0;
}

__attribute__((section(".data.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
