#ifndef MZ_HXX
#define MZ_HXX 1

#include <cstdint>
#include <cstddef>
#include "vendor.hxx"
#include "task.hxx"

#define MZ_SIGNATURE 0x5A4D // 'M','Z' in ASCII

namespace MZ
{
struct Header
{
    uint16_t signature;
    uint16_t bytes_last_page; // Bytes in last page
    uint16_t num_pages;       // Number of pages
    uint16_t rel_entries;     // Number of entries in relocation table
    uint16_t header_size;     // Paragraphs
    uint16_t min_alloc_para;  // Minimum allocation paragraphs (except PSP and program image)
    uint16_t max_alloc_para;  // Maximum allocation paragraph
    uint16_t initial_ss;
    uint16_t initial_sp;
    uint16_t checksum;
    uint16_t initial_ip;
    uint16_t initial_cs;
    uint16_t rel_table_offset; // Absolute offset to the relocation table
    uint16_t overlay;          // 0 = this is a executable
} PACKED;

struct RelEntry
{
    uint16_t offset;
    uint16_t segment;
} PACKED;

void Load(Task::TSS &task, unsigned char *buffer, size_t bufsize);
}

#endif
