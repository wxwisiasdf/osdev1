#include <cstdint>
#include <cstddef>
#include <cstring>
#include <new>
#include "alloc.hxx"
#include "tty.hxx"

static HimemAlloc::Manager g_KMMaster;

#define MAX_MEM_LOW 0x100000 // Max address range for low allocator
#define LOW_ALLOC_SIZE 512   // Allocates in blocks of 512 bytes
#define MAX_BLOCKS 512       // Max number of blocks

extern unsigned char lowMem[MAX_MEM_LOW];

unsigned char lowBitmap[(MAX_MEM_LOW / LOW_ALLOC_SIZE) / 8] = {
    0xF0, // 4, 512 byte paragraphs
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

struct LowMemBlock
{
    uint32_t addr_lo : 16;
    uint32_t addr_hi : 4;
    uint32_t n_para : 12;
};
static LowMemBlock lowUsedBlocks[MAX_BLOCKS] = {};

/// @brief Adds a block of memory to the used block list
/// @param addr Address of block
/// @param n_para Number of paragraphs taken by the block
void Alloc::AddUsedBlock(uintptr_t addr, size_t n_para)
{
    for (size_t i = 0; i < MAX_BLOCKS; i++)
    {
        auto *block = &lowUsedBlocks[i];
        if (block->n_para == 0)
        {
            block->addr_lo = addr;
            block->addr_hi = addr >> 16;
            block->n_para = n_para;
            for (size_t j = addr / LOW_ALLOC_SIZE;
                 j < (addr / LOW_ALLOC_SIZE) + n_para;
                 j++)
                Alloc::SetBitmap(lowBitmap, j, true);
            return;
        }
    }
}

void Alloc::SetBitmap(uint8_t bitmap[], size_t index, bool value)
{
    if (value)
        bitmap[index / 8] |= (1 << (index % 8));
    else
        bitmap[index / 8] &= ~(1 << (index % 8));
}

bool Alloc::GetBitmap(uint8_t bitmap[], size_t index)
{
    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

/// @brief Get memory from a low address
/// @param n_para Number of paragraphs
/// @return Address allocated
void *Alloc::GetLow(size_t n_para)
{
    uintptr_t address = 0;
    size_t rem_para = n_para; // Remaining paragraphs
    for (size_t i = 0; i < sizeof(lowBitmap) * 8 && rem_para; i++)
    {
        // Location is in use?
        if (Alloc::GetBitmap(lowBitmap, i))
        {
            rem_para = n_para; // Stride broken, reset
            continue;
        }
        if (rem_para == n_para) // Update address
            address = i * LOW_ALLOC_SIZE;
        rem_para--;
    }

    if (rem_para)
        return nullptr;

    Alloc::AddUsedBlock(address, n_para);
    return (void *)address;
}

void *Alloc::ResizeLow(void *addr, size_t para)
{
    auto *p = Alloc::GetLow(para);
    for (size_t i = 0; i < para * LOW_ALLOC_SIZE; i++)
        ((uint8_t *)p)[i] = ((uint8_t *)addr)[i];
    Alloc::FreeLow(addr);
    return p;
}

void Alloc::FreeLow(void *addr)
{
    for (size_t i = 0; i < MAX_BLOCKS; i++)
    {
        auto *block = &lowUsedBlocks[i];
        uintptr_t linear_addr = (block->addr_hi << 16) | block->addr_lo;
        if (linear_addr == (uintptr_t)addr)
        {
            for (size_t j = linear_addr / LOW_ALLOC_SIZE;
                 j < (linear_addr / LOW_ALLOC_SIZE) + block->n_para;
                 j++)
                Alloc::SetBitmap(lowBitmap, j, false);
            return;
        }
    }
}

/* SOF allocator - this allocator, designed by me :D . Uses a slab for small
 * objects. Supports RAM holes and RAM hotplug.
 *
 * Allocation is done from end to start. This is because software will always
 * hit the free block. Making allocation be O(1) for most of the time.
 *
 * The heap is "infinite" in the sense that we really never expand it. Since
 * the free block always is the fist block; the heap *will* take space
 * from the free block. This reduces complexity by a fuckton. */

// TODO: Add poison
// TODO: We can run out of region space; We need a way to allocate extra master blocks.
// TODO: We need to also consider that RAM hotswap is a thing; so we need to copy master blocks.
// TODO: We need to make copies of the master RAM block; because RAM hotswap exists

HimemAlloc::Manager& HimemAlloc::Manager::GetDefault()
{
    return g_KMMaster;
}

int HimemAlloc::InitManager(HimemAlloc::Manager& master)
{
    master.next = nullptr;
    master.max_regions = DEFAULT_MAX_REGIONS;
    for (size_t i = 0; i < master.max_regions; i++)
    {
        auto &region = master.regions[i];
        region.addr = nullptr;
        region.free_size = 0;
        region.size = 0;
        region.head = nullptr;
        region.max_blocks = 0; /* Default for new blocks */
        region.blocks = nullptr;
    }
    return 0;
}

HimemAlloc::Region *HimemAlloc::AddRegion(HimemAlloc::Manager& master, void *addr, size_t size)
{
    HimemAlloc::Region *region = nullptr;
    /* Record new region onto the master heap */
    for (size_t i = 0; i < master.max_regions; i++)
    {
        region = &master.regions[i];
        if (region->addr == nullptr)
            break;
    }

    if (region == nullptr)
        return nullptr;

    /* Fill out info of region */
    region->addr = (char *)addr;
    region->free_size = size;
    region->size = size;
    region->blocks = (HimemAlloc::Block *)addr;
    region->head = (HimemAlloc::Block *)addr;
    region->max_blocks = size / DEFAULT_DESCRIPTORS_PER_BYTES;
    if (region->max_blocks < 8192)
        region->max_blocks = 8192;

    /* Fill heap correctly */
    for (size_t i = 0; i < region->max_blocks; i++)
    {
        auto &tmp = region->blocks[i];
        tmp.type = HimemAlloc::Block::Type::NOT_PRESENT;
    }

    /* Create the genesis block - this block is used for ram */
    auto &block = region->blocks[0];
    block.next = nullptr;
    block.type = HimemAlloc::Block::Type::FREE;

    /* New genesis block = less free memory on region */
    region->free_size -= sizeof(HimemAlloc::Block) * region->max_blocks;
    region->blocks[0].size = region->free_size;
    return region;
}

HimemAlloc::Block *HimemAlloc::AddBlock(HimemAlloc::Region& region)
{
    /* Iterate through the blocks on heap, not all blocks are used
     * and some are just there to be took over by newer blocks */
    for (size_t i = 0; i < region.max_blocks; i++)
    {
        auto &block = region.blocks[i];
        if (block.type == HimemAlloc::Block::Type::NOT_PRESENT)
            return &block;
    }

    // TODO: We could just expand the heap to avoid less errors.
    TTY::Print("No free blocks found\n");
    return nullptr;
}

/** Fixes a block by merging next blocks into it (incase this block is a free block) */
static inline void mem_fix_block(auto& block)
{
    if (block.next != nullptr && block.type == HimemAlloc::Block::Type::FREE && block.next->type == HimemAlloc::Block::Type::FREE)
    {
        block.size += block.next->size;
        block.next->type = HimemAlloc::Block::Type::NOT_PRESENT;
        block.next = block.next->next;
    }
}

/** Allocate memory of specified size */
void *HimemAlloc::Alloc(HimemAlloc::Manager &master, size_t size)
{
    for (size_t i = 0; i < master.max_regions; i++)
    {
        auto &region = master.regions[i];
        if (region.addr == nullptr || region.free_size < size)
            continue;

        auto *block = region.head;
        char *ptr = region.addr;
        while (block != nullptr)
        {
            mem_fix_block(*block);

            if (block->type != HimemAlloc::Block::Type::FREE && !(block->size >= size))
            {
                ptr += block->size;
                block = block->next;
                continue;
            }

            HimemAlloc::Block *newblock;
            if ((newblock = HimemAlloc::AddBlock(region)) == nullptr)
                return nullptr;

            newblock->type = HimemAlloc::Block::Type::USED;

            /* Parenting - VERY IMPORTANT! */
            newblock->next = block->next;
            block->next = newblock;

            newblock->size = size;
            block->size -= size;
            region.free_size -= size;

            ptr += block->size;
            return ptr;
        }
    }
    return nullptr;
}

/** Free previously allocated memory */
void HimemAlloc::Free(HimemAlloc::Manager &master, void *ptr)
{
    for (size_t i = 0; i < master.max_regions; i++)
    {
        auto &region = master.regions[i];
        char *bptr = region.addr;
        /* While we recurse we will also take the opportunity to
         * merge free blocks */
        HimemAlloc::Block *block = region.head, *prev = nullptr;
        while (block != nullptr)
        {
            mem_fix_block(*block);

            if (block->type == HimemAlloc::Block::Type::USED && bptr >= ptr && ptr <= bptr + block->size)
            {
                if (prev != nullptr && prev->type == HimemAlloc::Block::Type::FREE)
                {
                    prev->next = block->next;
                    prev->size += block->size;
                    block->type = HimemAlloc::Block::Type::NOT_PRESENT;
                }
                else
                {
                    block->type = HimemAlloc::Block::Type::FREE;
                }
                return;
            }

            bptr += block->size;

            prev = block;
            block = block->next;
        }
    }
}

/** Allocate memory with align constraint */
void *HimemAlloc::AlignAlloc(HimemAlloc::Manager &master, size_t size, size_t align)
{
    for (size_t i = 0; i < master.max_regions; i++)
    {
        auto &region = master.regions[i];
        if (region.addr == nullptr || region.free_size < size)
            continue;
        
        auto *block = region.head;
        char *bptr = region.addr;
        while (block != nullptr)
        {
            uintptr_t pbptr = (uintptr_t)bptr;
            if (block->type == HimemAlloc::Block::Type::FREE && block->size >= size + ((uintptr_t)pbptr % align))
            {
                uintptr_t target;
                /* Perfect match! (very unlikely) */
                if (!(uintptr_t)pbptr % align)
                {
                    /* If size is bigger split block into free and used parts */
                    if (block->size >= align)
                    {
                        /* New block AFTER this used block */

                        // TODO: Make block be BEFORE this used block (for speed reasons)
                        HimemAlloc::Block *newblock;
                        newblock = HimemAlloc::AddBlock(region);
                        if (newblock == nullptr)
                            return nullptr;

                        block->type = HimemAlloc::Block::Type::USED;
                        newblock->next = block->next;
                        block->next = newblock;

                        newblock->size = block->size - size;
                        block->size = size;
                        return bptr;
                    }
                    /* Astronomically impossible: the perfectest match, aligned and
                     * with same size */
                    else if (block->size == size)
                    {
                        block->type = HimemAlloc::Block::Type::USED;
                        return bptr;
                    }
                }
                /* Block sufficiently big */
                else
                {
                    target = pbptr + block->size;
                    target -= align;
                    target -= target % align;

                    uintptr_t off = target - pbptr;

                    /* First check if we should make  afree part AFTER this block (a free part) */
                    if (off + size > block->size)
                    {
                        HimemAlloc::Block *afterblock;
                        afterblock = HimemAlloc::AddBlock(region);
                        if (afterblock == nullptr)
                            return nullptr;
                        afterblock->size = block->size - (off + size);
                        afterblock->type = HimemAlloc::Block::Type::FREE;

                        afterblock->next = block->next;
                        block->next = afterblock;
                        block->size -= afterblock->size;
                    }

                    // TODO: Assert off != 0

                    /* Make block BEFORE this block (also a free part) */
                    HimemAlloc::Block *usedblock;
                    usedblock = HimemAlloc::AddBlock(region);
                    if (usedblock == nullptr)
                        return nullptr;
                    block->size = off;
                    usedblock->size = size;
                    usedblock->type = HimemAlloc::Block::Type::USED;

                    usedblock->next = block->next;
                    block->next = usedblock;

                    // TODO: Implement after eating pizza
                    return (void *)(pbptr + block->size);
                }
            }
            bptr += block->size;
        }
    }
    return nullptr;
}

/** Reallocate previously allocated memory */
void *HimemAlloc::Realloc(HimemAlloc::Manager &master, void *ptr, size_t size)
{
    char *bptr;
    for (size_t i = 0; i < master.max_regions; i++)
    {
        auto &region = master.regions[i];
        HimemAlloc::Block *block, *prev;
        if (region.addr == nullptr || region.free_size < size)
            continue;
        bptr = region.addr;
        block = region.head;
        prev = nullptr;
        while (block != nullptr)
        {
            mem_fix_block(*block);

            /* Find block - if possible take space from next free block.
             * Otherwise do a allocation and then free. */
            if (block->type == HimemAlloc::Block::Type::USED && bptr >= ptr && ptr <= bptr + block->size)
            {
                size_t diff;

                diff = size - block->size;

                /* Expand */
                if ((ssize_t)diff > 0)
                {
                    /* Take size from next block */
                    if (block->next != nullptr && block->next->type == HimemAlloc::Block::Type::FREE && block->next->size >= diff)
                    {
                        block->size += diff;
                        block->next->size -= diff;
                        region.free_size -= diff;
                        return bptr;
                    }
                    /* Take size from previous block */
                    else if (prev != nullptr && prev->type == HimemAlloc::Block::Type::FREE && prev->size >= diff)
                    {
                        block->size += diff;
                        prev->size -= diff;
                        region.free_size -= diff;
                        return bptr - diff;
                    }
                    /* If the method above fails; we will do a malloc and then
                     * free the old block */
                    else
                    {
                        void *new_ptr;
                        new_ptr = HimemAlloc::Alloc(master, size);
                        if (new_ptr == nullptr)
                            return nullptr;

                        memcpy(new_ptr, ptr, block->size);
                        HimemAlloc::Free(master, ptr);
                        return new_ptr;
                    }
                }
                /* Shrink */
                else if ((ssize_t)diff < 0)
                {
                    diff = -diff;

                    /* Give size to block before */
                    if (prev != nullptr && prev->type == HimemAlloc::Block::Type::FREE)
                    {
                        prev->size += diff;
                        block->size -= diff;
                        region.free_size += diff;
                        return bptr + diff;
                    }
                    /* Give size to block after */
                    else if (block->next != nullptr && block->next->type == HimemAlloc::Block::Type::FREE)
                    {
                        block->next->size += diff;
                        block->size -= diff;
                        region.free_size += diff;
                        return bptr;
                    }
                    /* Create (split) block with remainder size. We will put it after
                     * the block for simplicity */
                    // TODO: We must do it BEFORE block; because that reduces fragmentation
                    // remember that all free blocks are guaranteed to be at the left
                    else
                    {
                        auto *new_block = HimemAlloc::AddBlock(region);
                        if (new_block == nullptr)
                            return nullptr;

                        new_block->type = HimemAlloc::Block::Type::FREE;
                        new_block->size = diff;
                        block->size -= diff;
                        region.free_size += diff;
                        return bptr;
                    }
                }
                /* No change - no reallocation :D */
                else
                {
                    return bptr;
                }
            }

            bptr += block->size;
            prev = block;
            block = block->next;
        }
    }

    /* This only happens when */
    bptr = (char *)HimemAlloc::Alloc(master, size);
    return nullptr;
}

HimemAlloc::Info HimemAlloc::GetInfo(const HimemAlloc::Manager& master)
{
    HimemAlloc::Info info{};
    for (size_t i = 0; i < master.max_regions; i++)
    {
        const auto& region = master.regions[i];
        if (region.addr == nullptr)
            continue;
        
        auto *block = region.head;
        while (block != nullptr)
        {
            if (block->type == HimemAlloc::Block::Type::FREE)
                info.free += block->size;
            info.total += block->size;
            block = block->next;
        }
    }
    return info;
}

void HimemAlloc::Print(const HimemAlloc::Manager& master)
{
    TTY::Print("master has max. %u regions\n", master.max_regions);
    for (size_t i = 0; i < master.max_regions; i++)
    {
        auto& region = master.regions[i];
        if (region.addr == nullptr)
            continue;

        TTY::Print("Region#%u, size %u, maxBlocks %u, free %u, totalSize %u\n", i, region.size, region.max_blocks, region.free_size, region.size);
        
        auto *block = region.head;
        while (block != nullptr)
        {
            TTY::Print("%s %u\n", (block->type == HimemAlloc::Block::Type::FREE) ? "FREE" : "USED", block->size);
            block = block->next;
        }
    }
}

[[nodiscard]] void* operator new(std::size_t size)
{
    return HimemAlloc::Alloc(g_KMMaster, size);
}

[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment)
{
    return HimemAlloc::AlignAlloc(g_KMMaster, size, static_cast<size_t>(alignment));
}

[[nodiscard]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    return HimemAlloc::Alloc(g_KMMaster, size);
}

[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment,
                                 const std::nothrow_t&) noexcept
{
    return HimemAlloc::AlignAlloc(g_KMMaster, size, static_cast<size_t>(alignment));
}
 
void  operator delete(void* ptr) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete(void* ptr, std::size_t) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete(void* ptr, std::align_val_t) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete(void* ptr, std::size_t, std::align_val_t) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete(void* ptr, std::align_val_t,
                      const std::nothrow_t&) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}
 
[[nodiscard]] void* operator new[](std::size_t size)
{
    return HimemAlloc::Alloc(g_KMMaster, size);
}

[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return HimemAlloc::AlignAlloc(g_KMMaster, size, static_cast<size_t>(alignment));
}

[[nodiscard]] void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    return HimemAlloc::Alloc(g_KMMaster, size);
}

[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment,
                                   const std::nothrow_t&) noexcept
{
    return HimemAlloc::AlignAlloc(g_KMMaster, size, static_cast<size_t>(alignment));
}
 
void  operator delete[](void* ptr) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete[](void* ptr, std::size_t) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete[](void* ptr, std::align_val_t) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete[](void* ptr, std::align_val_t,
                        const std::nothrow_t&) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

#if 0
[[nodiscard]] void* operator new  (std::size_t size, void*) noexcept
{
    return HimemAlloc::Alloc(g_KMMaster, size);
}

[[nodiscard]] void* operator new[](std::size_t size, void*) noexcept
{
    return HimemAlloc::Alloc(g_KMMaster, size);
}

void  operator delete  (void* ptr, void*) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}

void  operator delete[](void* ptr, void*) noexcept
{
    HimemAlloc::Free(g_KMMaster, ptr);
}
#endif
