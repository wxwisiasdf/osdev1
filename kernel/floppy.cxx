#include <cstdint>
#include <cstddef>
#include "floppy.hxx"
#include "tty.hxx"
#include "vendor.hxx"

Floppy::Driver::Driver()
{
    // Obtain data from CMOS.
    // "High nibble contains the first drive information, lower one has
    // the second drive identifier"
    IO_Out8(0x70, 0x10);
    auto ch = IO_In8(0x71);
    drives[0] = static_cast<Floppy::Driver::DriveType>(ch >> 4);
    drives[1] = static_cast<Floppy::Driver::DriveType>(ch & 0x0F);
    TTY::Print("floppy: Drive 1=%i,2=%i\n", drives[0], drives[1]);
}
