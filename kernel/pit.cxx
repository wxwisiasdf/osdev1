module;

import pic;
#include "tty.hxx"
#include "task.hxx"

export module pit;

extern "C" void IntE8h_Handler()
{
    Task::DisableSwitch();
    //TTY::Print("pit: Handling interrupt E8\n");
    Task::Schedule();
    PIC::Get().EOI(0);
    Task::EnableSwitch();
}
