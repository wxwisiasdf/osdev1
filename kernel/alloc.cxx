#include <cstdint>
#include "alloc.hxx"

#define MAX_MEM_LOW 0x100000 // Max address range for low allocator
#define LOW_ALLOC_SIZE 512   // Allocates in blocks of 512 bytes
#define MAX_BLOCKS 512       // Max number of blocks

extern unsigned char low_mem[MAX_MEM_LOW];

unsigned char low_bitmap[(MAX_MEM_LOW / LOW_ALLOC_SIZE) / 8] = {
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
static LowMemBlock low_used_blocks[MAX_BLOCKS] = {};

/// @brief Adds a block of memory to the used block list
/// @param addr Address of block
/// @param n_para Number of paragraphs taken by the block
void Alloc::AddUsedBlock(uintptr_t addr, size_t n_para)
{
    for (size_t i = 0; i < MAX_BLOCKS; i++)
    {
        auto *block = &low_used_blocks[i];
        if (block->n_para == 0)
        {
            block->addr_lo = addr;
            block->addr_hi = addr >> 16;
            block->n_para = n_para;
            for (size_t j = addr / LOW_ALLOC_SIZE;
                 j < (addr / LOW_ALLOC_SIZE) + n_para;
                 j++)
                Alloc::SetBitmap(low_bitmap, j, true);
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
    for (size_t i = 0; i < sizeof(low_bitmap) * 8 && rem_para; i++)
    {
        // Location is in use?
        if (Alloc::GetBitmap(low_bitmap, i))
        {
            rem_para = n_para; // Stride broken, reset
            continue;
        }
        if (rem_para == n_para) // Update address
            address = i * LOW_ALLOC_SIZE;
        rem_para--;
    }

    if (rem_para)
        return NULL;
    
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
        auto *block = &low_used_blocks[i];
        uintptr_t linear_addr = (block->addr_hi << 16)
            | block->addr_lo;
        if (linear_addr == (uintptr_t)addr)
        {
            for (size_t j = linear_addr / LOW_ALLOC_SIZE;
                 j < (linear_addr / LOW_ALLOC_SIZE) + block->n_para;
                 j++)
                Alloc::SetBitmap(low_bitmap, j, false);
            return;
        }
    }
}
