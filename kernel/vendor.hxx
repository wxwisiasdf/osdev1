#ifndef VENDOR_HXX
#define VENDOR_HXX

#include <cstdint>

#define PACKED __attribute__((packed))
#define ALIGN(x) __attribute__((aligned(x)))

#ifdef __GNUC__
#define UBSAN_FUNC __attribute__((no_sanitize("undefined")))
#else
#define UBSAN_FUNC __attribute__((no_sanitize_undefined))
#error Define your macros here
#endif

#define ARRAY_SIZE(x) (sizeof((x)) / (sizeof((x)[0])))

static inline void IO_Out8(uint16_t port, uint8_t val)
{
    asm volatile("\toutb %0, %1"
                 :
                 : "a"(val), "Nd"(port));
}

static inline void IO_Out16(uint16_t port, uint16_t val)
{
    asm volatile("\toutw %0, %1"
                 :
                 : "a"(val), "Nd"(port));
}

static inline void IO_Out32(uint16_t port, uint32_t val)
{
    asm volatile("\toutl %0, %1"
                 :
                 : "a"(val), "Nd"(port));
}

static inline uint8_t IO_In8(uint16_t port)
{
    uint8_t ret;
    asm volatile("\tinb %1,%0\r\n"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

static inline uint16_t IO_In16(uint16_t port)
{
    uint16_t ret;
    asm volatile("\tinw %1,%0\r\n"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

static inline uint32_t IO_In32(uint16_t port)
{
    uint32_t ret;
    asm volatile("\tinl %1,%0\r\n"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

#define IO_Wait(...) \
    IO_Out8(0x80, 0);

/// @brief Waits until timeout happens
/// @tparam P Predicate to evaluate
/// @param usec Microseconds to wait
/// @param pred Predicate
/// @return false if timeout exceeded, true if predicate was true
namespace Task
{
    void Sleep(unsigned int usec);
}
template <typename P>
inline bool IO_TimeoutWait(int usec, P pred)
{
    int rem_usec = usec;
    while (rem_usec--)
    {
        if (pred())
            return true;
        Task::Sleep(static_cast<unsigned int>(1));
    }
    return false;
}

constexpr static uintptr_t ComputeLinearAddr(uint32_t segment, uint32_t offset)
{
    return segment * 16 + offset;
}

#endif
