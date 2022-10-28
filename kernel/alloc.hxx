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

#endif
