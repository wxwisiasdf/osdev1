#ifndef AUDIO_HXX
#define AUDIO_HXX 1

#include <cstddef>
#include "vendor.hxx"

#define MAX_HZ 48000

namespace Audio
{
    struct Sample
    {
        enum Format
        {
            NONE, // No format
            PCM_U8, // 8-bit unsigned PCM
        } fmt;
        char buffer[MAX_HZ] ALIGN(4096);
        size_t nBuffer;
    };

    struct Driver
    {
        Driver() = default;
        Driver(int _base, char inout, int bits);
        Driver(Driver&) = delete;
        Driver(Driver&&) = delete;
        Driver& operator=(Driver&) = delete;
        ~Driver() = default;

        virtual void Start();
        virtual void Pause();
        virtual void Resume();
        virtual void Finish();
    };
}

#endif
