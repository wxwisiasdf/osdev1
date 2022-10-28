#ifndef ASSERT_HXX
#define ASSERT_HXX 1

#include "tty.hxx"

#define assert(x)                             \
    do                                        \
    {                                         \
        if (!(x))                             \
        {                                     \
            TTY::Print("Assertion " #x "\n"); \
            while (1)                         \
                ;                             \
        }                                     \
    } while (0)

#endif
