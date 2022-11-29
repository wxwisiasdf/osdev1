#include "ps2.hxx"

PS2::Keyboard *PS2::g_ps2_keyboard = nullptr;
PS2::Mouse *PS2::g_ps2_mouse = nullptr;

/// @brief Keyboard IRQ handler
extern "C" void IntE9h_Handler()
{
    Task::DisableSwitch();
#ifdef DEBUG
    TTY::Print("ps2: Handling IRQ E9 for keyboard\n");
#endif
    auto &kb = PS2::Keyboard::Get();
    kb.buf[kb.n_buf++] = kb.GetPollKey();
    if (kb.n_buf >= sizeof(kb.buf))
        kb.n_buf = 0;
    kb.buf[kb.n_buf] = '\0';
    Task::Schedule();
    PIC::Get().EOI(1);
    Task::EnableSwitch();
}

/// @brief Mouse IRQ handler
extern "C" void IntF4h_Handler()
{
    Task::DisableSwitch();
#ifdef DEBUG
    TTY::Print("ps2: Handling IRQ F4 for mouse\n");
#endif
    auto &mouse = PS2::Mouse::Get();
    mouse.PollRead();
    if (mouse.x > g_KFrameBuffer.width - 8)
        mouse.x = g_KFrameBuffer.width - 8;
    if (mouse.y > g_KFrameBuffer.height - 8)
        mouse.y = g_KFrameBuffer.height - 8;
    g_KFrameBuffer.MoveMouse(mouse.GetX(), mouse.GetY());
    Task::Schedule();
    PIC::Get().EOI(12);
    Task::EnableSwitch();
}
