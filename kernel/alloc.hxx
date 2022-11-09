#ifndef ALLOC_HXX
#define ALLOC_HXX 1

#include <cstddef>
#include <cstdint>

/// @brief Low memory allocator
/// Allocates memory in the low address space so the DOS programs can run on
/// the given addresses, this allocator is pretty simple and henceforth it uses
/// a bitmap to track every allocation and a separate list of used blocks.
///
/// The allocator is NOT meant to be used by the OS for normal tasks, rather
/// it's meant to be used when the OS needs to load v86 programs and requires
/// to mimic MS-DOS behaviour by loading the programs on the low memory then
/// switching into v8086 mode.
namespace Alloc
{
    void AddUsedBlock(uintptr_t addr, size_t n_para);
    void SetBitmap(uint8_t bitmap[], size_t index, bool value);
    bool GetBitmap(uint8_t bitmap[], size_t index);
    void *GetLow(size_t para);
    void *ResizeLow(void *addr, size_t para);
    void FreeLow(void *addr);
}

/// @brief High memory allocator
namespace HimemAlloc
{
#define PAGE_SIZE 4096
#define DEFAULT_MAX_REGIONS 4
#define DEFAULT_DESCRIPTORS_PER_BYTES 4096

    struct Block
    {
        Block(Block &) = delete;
        Block(Block &&) = delete;
        Block &operator=(const Block &) = delete;

        struct Block *next;
        size_t size;
        enum Type
        {
            NOT_PRESENT = 0,
            FREE = 1,
            USED = 2,
            POISON = 4
        } type;
    };

    struct Region
    {
        Region() = default;
        Region(Region &) = delete;
        Region(Region &&) = delete;
        Region &operator=(const Region &) = delete;
        ~Region() = default;

        char *addr = nullptr;
        size_t free_size = 0;
        size_t size = 0;
        HimemAlloc::Block *head = nullptr;
        size_t max_blocks = 0;
        HimemAlloc::Block *blocks = nullptr;
    };

    // TODO: Do something when we run out of regions (make new regions)
    struct Manager
    {
        Manager() = default;
        Manager(Manager &) = delete;
        Manager(Manager &&) = delete;
        Manager &operator=(const Manager &) = delete;
        ~Manager() = default;

        Manager *next = nullptr;
        size_t max_regions = 0;
        HimemAlloc::Region regions[DEFAULT_MAX_REGIONS] = {};

        static Manager& GetDefault();
    };

    int InitManager(Manager& master);
    HimemAlloc::Region *AddRegion(Manager& master, void *addr, size_t size);
    HimemAlloc::Block *AddBlock(Region &region);
    void *Alloc(Manager &master, size_t size);
    void Free(Manager &master, void *ptr);
    void *AlignAlloc(Manager &master, size_t size, size_t align);
    void *Realloc(Manager &master, void *ptr, size_t size);
    struct Info
    {
        size_t total = 0;
        size_t free = 0;
    };
    
    Info GetInfo(const HimemAlloc::Manager &master);
    void Print(const HimemAlloc::Manager& master);
}

#endif
