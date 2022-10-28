#include "vga.hxx"
import locale;

VGA::VGA()
    : TTY::Terminal()
{
    this->rows = 25;
    this->cols = 80;
}

void VGA::PlotChar(char32_t ch)
{
    auto lch = Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>(ch);
    this->vga_text_addr[this->x + (this->y * this->cols)] = lch | (this->color << 8);
}
