/// calc.cxx
/// Calculator program

#include <optional>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> g_Desktop;
int UDOS_32Main(char32_t[])
{
    static std::optional<UI::Window> calcWindow;
    calcWindow.emplace();
    calcWindow->x = 0;
    calcWindow->y = 0;
    calcWindow->width = 80;
    calcWindow->height = 128;
    calcWindow->SetText("Calculator");
    g_Desktop->AddChildDirect(calcWindow.value());

    static std::optional<UI::Button> numBtns[10];
    static const char *digitList[] = {
        "7", "8", "9",
        "4", "5", "6",
        "1", "2", "3",
        "0",
    };

    for (size_t i = 0; i < 9; i++)
    {
        numBtns[i].emplace();
        numBtns[i]->x = (i == 0 ? 0 : i % 3) * 16;
        numBtns[i]->y = (i == 0 ? 0 : i / 3) * 16;
        numBtns[i]->width = 12;
        numBtns[i]->height = 12;
        numBtns[i]->SetText(digitList[i]);
        calcWindow->AddChildDirect(numBtns[i].value());
    }

    static std::optional<UI::Button> eqBtn;
    eqBtn.emplace();
    eqBtn->x = 16 * 3;
    eqBtn->y = 16;
    eqBtn->width = 12;
    eqBtn->height = 12 + 16;
    eqBtn->SetText("=");
    calcWindow->AddChildDirect(eqBtn.value());

    static std::optional<UI::Button> minusBtn;
    minusBtn.emplace();
    minusBtn->x = 16 * 3;
    minusBtn->y = 0;
    minusBtn->width = 12;
    minusBtn->height = 12;
    minusBtn->SetText("-");
    calcWindow->AddChildDirect(minusBtn.value());

    while (!calcWindow->isClosed)
        Task::Switch();
    return 0;
}
