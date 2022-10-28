#include <optional>
#include <type_traits>
import uart;
import locale;
#include <cstdint>
#include "tty.hxx"
#include "vendor.hxx"

TTY::Terminal::~Terminal()
{
    if (this->kernelManaged)
        this->DetachFromKernel(*this);
}

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

char *TTY::BufferTerminal::GetBuffer()
{
    return this->buffer;
}

void TTY::BufferTerminal::PlotChar(char32_t ch)
{
    this->buffer[this->nBuffer++] = ch;
    if (this->nBuffer >= sizeof(this->buffer))
        this->nBuffer = 0;
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

void TTY::Print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char tmpbuf[128] = {};
    TTY::Print_1(tmpbuf, sizeof(tmpbuf), fmt, args);

    for (size_t i = 0; i < ARRAY_SIZE(kernelTerms); i++)
        if (kernelTerms[i] != nullptr)
            kernelTerms[i]->Print(tmpbuf);
    va_end(args);
}
