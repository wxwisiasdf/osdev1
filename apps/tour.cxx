/// tour.cxx
/// Touring program

#include <optional>
#include <kernel/ui.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

extern std::optional<UI::Desktop> desktop;
int UDOS_32Main(char32_t[])
{
    static std::optional<UI::Window> tourWin;
    tourWin.emplace();
    tourWin->SetText("Tour to UDOS/3X");
    tourWin->x = tourWin->y = 0;
    tourWin->width = 250;
    tourWin->height = 150;
    tourWin->Decorate();
    desktop->AddChild(tourWin.value());

    static std::optional<UI::Textbox> tourTextbox;
    tourTextbox.emplace();
    tourTextbox->SetText(
        U"Hi, this is UDOS/3X\n"
        U"The best OS on Earth!\n"
        U"\x01F60D\x01F60D\x01F60D\x01F60D\x01F60D\x01F60D\x01F60D\x01F60D\n"
        U"\x01F618\x01F618\x01F618\x01F618\x01F618\x01F618"
        U"<3 <3 <3 <3 <3 <3\n"
        U"Just click on stuff...\n"
        U"Have fun! :)\n"
        U"- leaf"
    );
    tourTextbox->x = tourTextbox->y = 0;
    tourTextbox->width = tourWin->width - 12;
    tourTextbox->height = tourWin->height - 32;
    tourWin->AddChild(tourTextbox.value());

    static std::optional<UI::Button> mysteryBtn[4];
    mysteryBtn[0].emplace();
    mysteryBtn[0]->SetText("1");
    mysteryBtn[0]->x = 64;
    mysteryBtn[0]->y = -16;
    mysteryBtn[0]->width = 12;
    mysteryBtn[0]->height = 12;
    mysteryBtn[0]->SetTooltipText(U"A mystery button!, what will it do? \x01F60A");
    mysteryBtn[0]->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
        g_KFrameBuffer.mouseMode = Framebuffer::MouseMode::WAIT;
    });
    tourWin->AddChild(mysteryBtn[0].value());

    mysteryBtn[1].emplace();
    mysteryBtn[1]->SetText("2");
    mysteryBtn[1]->x = 64 + 16;
    mysteryBtn[1]->y = -16;
    mysteryBtn[1]->width = 12;
    mysteryBtn[1]->height = 12;
    mysteryBtn[1]->SetTooltipText(U"A mystery button!, what will it do? \x01F60A");
    mysteryBtn[1]->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
        tourTextbox->SetText(U"Fuck monolithic kernels");
    });
    tourWin->AddChild(mysteryBtn[1].value());

    mysteryBtn[2].emplace();
    mysteryBtn[2]->SetText("3");
    mysteryBtn[2]->x = 64 + 16 + 16;
    mysteryBtn[2]->y = -16;
    mysteryBtn[2]->width = 12;
    mysteryBtn[2]->height = 12;
    mysteryBtn[2]->SetTooltipText(U"A mystery button!, what will it do? \x01F60A");
    mysteryBtn[2]->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
        desktop->background = UI::Desktop::Background::DIAGONAL_LINES;
        desktop->primaryColor = Color(desktop->primaryColor.rgba + 0x46323863);
        desktop->secondaryColor = Color(desktop->primaryColor.rgba + 0x69420124);
        desktop->Redraw();
    });
    tourWin->AddChild(mysteryBtn[2].value());

    mysteryBtn[3].emplace();
    mysteryBtn[3]->SetText("4");
    mysteryBtn[3]->x = 64 + 16 + 16 + 16;
    mysteryBtn[3]->y = -16;
    mysteryBtn[3]->width = 12;
    mysteryBtn[3]->height = 12;
    mysteryBtn[3]->SetTooltipText(U"A mystery button!, what will it do? \x01F60A");
    mysteryBtn[3]->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
        desktop->background = UI::Desktop::Background::MULTIPLY_OFFSET;
        desktop->primaryColor = Color(desktop->primaryColor.rgba + 0x46323863);
        desktop->secondaryColor = Color(desktop->primaryColor.rgba + 0x69420124);
        desktop->Redraw();
    });
    tourWin->AddChild(mysteryBtn[3].value());

    while (!tourWin->isClosed)
        Task::Switch();
    return 0;
}
