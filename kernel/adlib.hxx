#ifndef ADLIB_HXX
#define ADLIB_HXX 1

#include <cstdint>
#include <cstddef>

namespace AdLib
{
class Device
{
    void Write(unsigned char reg, unsigned char val) const;
public:
    Device();
    ~Device() = default;

    bool IsPresent() const;
    //void InitChannel(int channel);
    void Resume();
    void Pause();
    void PlayNote(int channel, unsigned short freq, unsigned char octave);
};
}

#endif
