#ifndef VGA_HXX
#define VGA_HXX 1

#include <cstdint>
#include "tty.hxx"

struct VGA : public TTY::Terminal
{
    volatile uint16_t *vga_text_addr = (volatile uint16_t *)0xB8000L;
    enum VGA_Color
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
    int color = 0x0F;

    VGA();
    VGA(VGA&) = delete;
    VGA(VGA&&) = delete;
    VGA& operator=(const VGA&) = delete;
    ~VGA() = default;
    void PlotChar(char32_t ch);
};

#endif
