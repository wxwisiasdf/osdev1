#ifndef GDT_HXX
#define GDT_HXX 1

#include <cstdint>
#include "vendor.hxx"

#define MAX_GDT_ENTRIES (8192 - 1)

namespace GDT
{
// Hardcoded segment selectors
constexpr auto KERNEL_XCODE = 0x08;
constexpr auto KERNEL_XDATA = 0x10;
constexpr auto USER_XCODE = 0x18;
constexpr auto USER_XDATA = 0x20;
constexpr auto LDT_SEGMENT = (1 << 2);

struct Desc
{
    uint16_t size = 0;
    uint32_t offset = 0;

    constexpr Desc() = default;
    ~Desc() = default;
} PACKED ALIGN(8);

struct Entry
{
    static constexpr auto TYPE_16TSS_AVAIL = 0x01;
    static constexpr auto TYPE_LDT = 0x02;
    static constexpr auto TYPE_16TSS_BUSY = 0x03;
    static constexpr auto TYPE_32TSS_AVAIL = 0x09;
    static constexpr auto TYPE_32TSS_BUSY = 0x0B;

    uint16_t limit_lo = 0;
    uint16_t base_lo = 0;
    uint8_t base_mid = 0;
    struct
    {
        uint8_t accessed : 1;
        uint8_t read_write : 1;
        uint8_t direction : 1;
        uint8_t executable : 1;
        uint8_t always_1 : 1;
        uint8_t privilege : 2; // 0 = kernel, 3 = usermode
        uint8_t present : 1;   // Must be 1
    } access = {};
    struct
    {
        uint8_t limit_hi : 4;
        uint8_t always_00 : 2;
        uint8_t size : 1;        // 0 = 16-bit pmode, 1 = 32-bit pmode
        uint8_t granularity : 1; // 0 = limit in 1byte granularity, 1 = limit in
        // 4kb granularity
    } flags = {};
    uint8_t base_hi = 0;

    constexpr Entry() = default;
    ~Entry() = default;

    void SetFlags(uint8_t c)
    {
        this->flags.always_00 = 0;
        this->flags.size = c >> 2;
        this->flags.granularity = c >> 3;
    }

    void SetAccess(uint8_t c)
    {
        this->access.accessed = c >> 0; // Should always be 0
        this->access.read_write = c >> 1;
        this->access.direction = c >> 2;
        this->access.executable = c >> 3;
        this->access.always_1 = c >> 4;
        this->access.privilege = c >> 5;
        this->access.present = c >> 7;
    }

    void SetLimit(uint32_t limit)
    {
        if (limit > 0xFFFFF)
        {
            this->flags.granularity = 1;
            //limit >>= 12; // Divide by 4096
        }
        this->limit_lo = limit;
        this->flags.limit_hi = limit >> 16;
    }

    void SetBase(void *base)
    {
        uintptr_t ptr = (uintptr_t)base;
        this->base_lo = ptr & 0xffff;
        this->base_mid = (ptr >> 16) & 0xff;
        this->base_hi = (ptr >> 24) & 0xff;
    }
} PACKED ALIGN(8);
void Init();
GDT::Entry &AllocateEntry();
int GetEntrySegment(const GDT::Entry &entry);
void SetupTSS();
GDT::Entry &GetEntry(unsigned segment);
void Reload();
}

namespace LDT
{
struct Entry : public GDT::Entry
{

} PACKED ALIGN(8);
}

struct IDT_Desc
{
    uint16_t size;
    uint32_t offset;
} PACKED ALIGN(8);

struct IDT_Entry
{
    static constexpr auto GATE_16TSS = 0x01;
    static constexpr auto GATE_16CALL = 0x04;
    static constexpr auto GATE_TASK = 0x05;
    static constexpr auto GATE_16INT = 0x06;
    static constexpr auto GATE_16TRAP = 0x07;
    static constexpr auto GATE_32CALL = 0x0C;
    static constexpr auto GATE_32INT = 0x0E;
    static constexpr auto GATE_32TRAP = 0x0F;

    uint16_t offset_lo;
    uint16_t seg_sel;
    uint8_t reserved = 0; // Always 0
    struct Flags
    {
        uint8_t gate_type : 4;
        uint8_t always_0 : 1;
        uint8_t privilege : 2; // Privilege
        uint8_t present : 1;
    } flags;
    uint16_t offset_hi;

    void set_offset(uint32_t offset)
    {
        this->offset_lo = offset & 0xffff;
        this->offset_hi = (offset >> 16) & 0xffff;
    }
} PACKED ALIGN(8);

namespace IDT
{
void AddHandler(int n, void (*fn)(void));
void RemoveHandler(int n, void (*fn)(void));
void SetEntry(int n, void (*fn)(void));
void Init();
void SetTaskSegment(int seg);
}

#endif
