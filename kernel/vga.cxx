#include <cstring>
#include "vga.hxx"
#include "locale.hxx"

VGATerm::VGATerm()
    : TTY::Terminal()
{
    this->rows = 25;
    this->cols = 80;
    std::memset((void *)this->vgaTextAddr, 0x00, this->cols * this->cols * sizeof(uint16_t));
}

void VGATerm::Newline()
{
    if(this->y >= this->rows)
    {
        std::memset((void *)&this->vgaTextAddr[(this->rows - 1) * this->cols * sizeof(uint16_t)], 0, this->cols * sizeof(uint16_t));
        std::memmove((void *)&this->vgaTextAddr[0], (const void *)&this->vgaTextAddr[this->cols], (this->rows - 1) * this->cols * sizeof(uint16_t));
        this->y = this->rows - 1;
    }
    this->modified = true;
}

void VGATerm::Putc(char32_t ch)
{
    switch (ch)
    {
    case '\t':
        this->x += this->tabsize - (this->x % this->tabsize);
        break;
    case '\n':
        this->x = 0;
        this->y++;
        this->Newline();
        break;
    case '\r':
        this->x = 0;
        break;
    default:
        this->PlotChar(ch);
        this->x++;
        break;
    }
    this->modified = true;
}

void VGATerm::PlotChar(char32_t ch)
{
    auto lch = Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>(ch);
    this->vgaTextAddr[this->x + (this->y * this->cols)] = lch | (this->color << 8);
    this->modified = true;
}
