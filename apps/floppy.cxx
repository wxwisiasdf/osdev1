#include <cstdint>
#include <cstddef>
#include <kernel/appkit.hxx>
#include <kernel/tty.hxx>
#include <kernel/vendor.hxx>

namespace Floppy
{
struct Driver
{
    /// @brief In accordance with CMOS values that can be readback
    enum DriveType
    {
        NONE = 0,
        F360KB = 1,
        F1_2MB = 2,
        F720KB = 3,
        F1_44MB = 4,
        F2_88MB = 5,
    } drives[2] = { NONE, NONE };

    Driver();
    Driver(Driver&) = delete;
    Driver(Driver&&) = delete;
    Driver& operator=(const Driver&) = delete;
    ~Driver() = default;
};
}

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

int UDOS_32Main(char32_t[])
{
    TTY::Print("floppy driver :D\n");
    return 0;
}

__attribute__((section(".text.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
