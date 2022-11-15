/// calc.cxx
/// Calculator program

#include <optional>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>
#include <kernel/alloc.hxx>

extern std::optional<UI::Desktop> g_Desktop;
int UDOS_32Main(char32_t[])
{
    auto& calcWindow = g_Desktop->AddChild<UI::Window>();
    calcWindow.x = 0;
    calcWindow.y = 0;
    calcWindow.width = 80;
    calcWindow.height = 128;
    calcWindow.SetText("Calculator");

    static const char *digitList[] = {
        "7", "8", "9",
        "4", "5", "6",
        "1", "2", "3",
        "0",
    };
    for (size_t i = 0; i < 9; i++)
    {
        auto& numBtn = calcWindow.AddChild<UI::Button>();
        numBtn.x = (i == 0 ? 0 : i % 3) * 16;
        numBtn.y = (i == 0 ? 0 : i / 3) * 16;
        numBtn.width = 12;
        numBtn.height = 12;
        numBtn.SetText(digitList[i]);
    }

    auto& eqBtn = calcWindow.AddChild<UI::Button>();
    eqBtn.x = 16 * 3;
    eqBtn.y = 16;
    eqBtn.width = 12;
    eqBtn.height = 12 + 16;
    eqBtn.SetText("=");

    auto& minusBtn = calcWindow.AddChild<UI::Button>();
    minusBtn.x = 16 * 3;
    minusBtn.y = 0;
    minusBtn.width = 12;
    minusBtn.height = 12;
    minusBtn.SetText("-");
    while (!calcWindow.isClosed)
        Task::Switch();
    calcWindow.Kill();
    return 0;
}
