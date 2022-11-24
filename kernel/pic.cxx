#include "pic.hxx"

PIC PIC::pic;

extern "C" void IntF5h_Handler()
{
    PIC::Get().EOI(13);
}
