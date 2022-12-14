#include <optional>
#include <string>
#include <type_traits>
#include "uart.hxx"
#include "locale.hxx"
#include <cstdint>
#include "tty.hxx"
#include "vendor.hxx"

void TTY::Terminal::Putc(char32_t ch)
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

void TTY::Terminal::Print(const char *str)
{
    for (; *str != '\0'; str++)
        this->Putc((*str) & 0xFF);
}

void TTY::Terminal::PlotChar(char32_t ch)
{
    UART::com[0]->Send(Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>(ch));
    this->modified = true;
}

void TTY::Terminal::Newline()
{
    UART::com[0]->Send(Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>('\n'));
    UART::com[0]->Send(Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>('\r'));
    this->modified = true;
}

#define MAX_ATTACHED_TERMINALS 8
static TTY::Terminal *kernelTerms[MAX_ATTACHED_TERMINALS] = {};
void TTY::Terminal::AttachToKernel(TTY::Terminal &term)
{
    for (size_t i = 0; i < ARRAY_SIZE(kernelTerms); i++)
    {
        if (kernelTerms[i] == nullptr)
        {
            kernelTerms[i] = &term;
            term.kernelManaged = true;
            return;
        }
    }
}

void TTY::Terminal::DetachFromKernel(TTY::Terminal &term)
{
    if (!term.kernelManaged)
        return;

    for (size_t i = 0; i < ARRAY_SIZE(kernelTerms); i++)
    {
        if (kernelTerms[i] == &term)
        {
            kernelTerms[i] = nullptr;
            return;
        }
    }
}

TTY::BufferTerminal::BufferTerminal()
    : TTY::Terminal()
{
}

void TTY::BufferTerminal::Putc(char32_t ch)
{
    UART::com[0]->Send(Locale::Convert<Locale::Charset::NATIVE, Locale::Charset::ASCII>(ch));
    TTY::BufferTerminal::PlotChar(ch);
}

const char *TTY::BufferTerminal::GetBuffer()
{
    return this->buffer.data();
}

void TTY::BufferTerminal::PlotChar(char32_t ch)
{
    if(!this->buffer.empty() && this->buffer[this->buffer.size() - 1] == '\0')
        this->buffer[this->buffer.size() - 1] = ch;
    else
        this->buffer.push_back(ch);
}

void TTY::BufferTerminal::Newline()
{
    // No-op
}

int TTY::ToChar(int digit)
{
    return Locale::ToChar(digit);
}

void TTY::Print_1(char *buffer, size_t size, const char *fmt, va_list args)
{
    while (*fmt)
    {
        for (; (*fmt != '%' && *fmt) && size; size--)
            *buffer++ = *fmt++;

        if (!size)
            break;

        if (*fmt == '%')
        {
            fmt++;
            if (*fmt == 'i')
                TTY::PrintIntegral<int>(&buffer, size, va_arg(args, int));
            else if (*fmt == 'u')
                TTY::PrintIntegral<unsigned>(&buffer, size, va_arg(args, unsigned));
            else if (*fmt == 'p' || *fmt == 'x')
                TTY::PrintIntegral<uintptr_t>(&buffer, size, va_arg(args, uintptr_t), 16);
            else if (*fmt == 'c')
                *buffer++ = static_cast<char>(va_arg(args, int));
            else if (*fmt == 's')
            {
                const auto *s = va_arg(args, const char *);
                for (; *s && size; size--)
                    *buffer++ = *s++;
            }
            fmt++;
        }
    }
    *buffer++ = '\0';
}

void TTY::Vprint(const char *fmt, va_list ap)
{
    char tmpbuf[128] = {};
    TTY::Print_1(tmpbuf, sizeof(tmpbuf), fmt, ap);
    for (size_t i = 0; i < ARRAY_SIZE(kernelTerms); i++)
        if (kernelTerms[i] != nullptr)
            kernelTerms[i]->Print(tmpbuf);
}

void TTY::Print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    TTY::Vprint(fmt, ap);
    va_end(ap);
}

void TTY::Print(const char32_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    //std::string normalFmt;
    //while (*fmt != U'\0')
    //    normalFmt.push_back(static_cast<char>(*fmt));
    //TTY::Vprint(normalFmt.c_str(), ap);
    va_end(ap);
}

