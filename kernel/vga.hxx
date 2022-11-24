#ifndef VGA_HXX
#define VGA_HXX 1

#include <cstdint>
#include "tty.hxx"

struct VGATerm : public TTY::Terminal
{
    volatile uint16_t *vgaTextAddr = (volatile uint16_t *)0xB8000L;
    enum VGAColor
    {
        BLACK,
        BLUE,
        GREEN,
        CYAN,
        RED,
        MAGNETA,
        PINK,
        BROWN,
        GRAY,
        DARK_GRAY,
        PURPLE,
        LIGHT_GREEN,
        LIGHT_BLUE,
        LIGHT_RED,
        LIGHT_PINK,
        YELLOW,
        WHITE
    };
    uint8_t color = 0x0F;

    VGATerm();
    VGATerm(VGATerm&) = delete;
    VGATerm(VGATerm&&) = delete;
    VGATerm& operator=(const VGATerm&) = delete;
    virtual ~VGATerm() = default;
    void Newline();
    void Putc(char32_t ch);
    void PlotChar(char32_t ch);
};

#endif
