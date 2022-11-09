#ifndef TTY_HXX
#define TTY_HXX 1

#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <type_traits>
#include <atomic>

#define MAX_TERM_BUF 512

namespace TTY
{
    class Terminal
    {
        bool kernelManaged = false;

    protected:
        virtual void PlotChar(char32_t ch);
        virtual void Newline();

    public:
        int tabsize = 4;
        int cols = 0;
        int rows = 0;
        int x = 0;
        int y = 0;
        std::atomic<bool> modified;

        Terminal() = default;
        Terminal(Terminal &) = delete;
        Terminal(Terminal &&) = delete;
        Terminal &operator=(const Terminal &) = delete;
        ~Terminal();

        virtual void Putc(char32_t ch);
        void Print(const char *str);

        static void AttachToKernel(Terminal& term);
        static void DetachFromKernel(Terminal& term);
    };

    class BufferTerminal : public Terminal
    {
    protected:
        char buffer[MAX_TERM_BUF] = {};
        unsigned nBuffer = 0;

    public:
        BufferTerminal();
        BufferTerminal(BufferTerminal &) = delete;
        BufferTerminal(BufferTerminal &&) = delete;
        BufferTerminal &operator=(const BufferTerminal &) = delete;
        ~BufferTerminal() = default;
        char *GetBuffer();
        void Putc(char32_t ch);
        void PlotChar(char32_t ch);
        void Newline();
    };

    int ToChar(int digit);
    template <typename T>
    void PrintIntegral(char **buffer, size_t size, T value, int base = 10)
    {
        if (!size)
            return;

        if constexpr (std::is_signed_v<T>)
            if (value < 0)
                *(*buffer)++ = '-';

        if (!value)
        {
            *(*buffer)++ = '0';
            return;
        }

        size_t digits = 0;
        for (T value2{value}; value2; value2 /= base, digits++)
            ;
        char *end_buffer = &((*buffer)[digits > size ? size : digits]);
        *buffer = end_buffer;
        // TODO: this breaks with UCS2
        for (; value && size; value /= base, size--)
            *--end_buffer = ToChar(value % base);
    }

    void Print_1(char *buffer, size_t size, const char *fmt, va_list args);
    void Print(const char *fmt, ...);
    void Print(const char32_t *fmt, ...);
}

#endif
