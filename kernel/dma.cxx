module;

#include <cstdint>
#include <cstddef>
#include "vendor.hxx"

export module dma;

export namespace DMA
{
enum ChanMode
{
    READ = 0x48,
    WRITE = 0x44,
    AUTO = 0x58,
};

struct Bus
{
    static void StartTransfer(void *data, size_t len, unsigned char channel, ChanMode chanMode)
    {
        if (channel <= 3)
            IO_Out8(0x0A, 4 + channel);
        else
            IO_Out8(0xD4, channel);

        IO_Out8(0x0C, 0x00);
        IO_Out8((channel <= 3) ? 0x0B : 0xD6, chanMode + channel);

        uintptr_t addr = (uintptr_t)data & 0xFFFE;
        static const uint8_t ports[8][3] =
        {
            { 0x87, 0x00, 0x01 },
            { 0x83, 0x02, 0x03 },
            { 0x81, 0x04, 0x05 },
            { 0x82, 0x06, 0x07 },
            { 0x8F, 0xC0, 0xC2 },
            { 0x8B, 0xC4, 0xC6 },
            { 0x89, 0xC8, 0xCA },
            { 0x8A, 0xCC, 0xCE },
        };
        IO_Out8(ports[channel][0], ((addr >> 16) & 0xFF));
        IO_Out8(ports[channel][1], (addr & 0xFF));
        IO_Out8(ports[channel][1], ((addr >> 8) & 0xFF));
        IO_Out8(ports[channel][2], (len & 0xFF));
        IO_Out8(ports[channel][2], ((len >> 8) & 0xFF));

        if (channel <= 3)
        {
            IO_Out8(0x0A, channel);
        }
        else
        {
            // Set the channel along with the mask which has to be set before reprogramming
            IO_Out8(0xD4, (1 << 2) | (channel - 4));
        }
    }
};
}
