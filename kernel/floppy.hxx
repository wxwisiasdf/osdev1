#ifndef FLOPPY_HXX
#define FLOPPY_HXX 1

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

#endif
