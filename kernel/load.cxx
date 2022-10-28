#include "load.hxx"
#include "alloc.hxx"
#include "tty.hxx"

void MZ::Load(Task::TSS &task, unsigned char *buffer, size_t bufsize)
{
    auto *header = (MZ::Header *)buffer;
    if (header->signature != MZ_SIGNATURE) // Invalid
        __builtin_unreachable();

    TTY::Print("mz_load: HeaderSize=%u (%u bytes)\n", header->header_size, header->header_size * 16);

    unsigned start_segment = 0x500;
    uint8_t *lin_code = (uint8_t *)((start_segment + header->initial_cs) * 16 + header->initial_ip);

    // Now patch the program image with the relocations
    auto *rel = (MZ::RelEntry *)&buffer[header->rel_table_offset];
    for (size_t i = 0; i < header->rel_entries; i++)
    {
        size_t offset = rel->segment * 16 + rel->offset;
        *((uint16_t *)(&buffer[offset])) += start_segment;
        rel++;
    }

    auto *program = &buffer[header->header_size * 16];
    TTY::Print("mz_load: CS:IP=%p:%p,SP=%p:%p\n", header->initial_cs, header->initial_ip, header->initial_ss, header->initial_sp);
    for (size_t i = 0; i < bufsize; i++)
        lin_code[i] = program[i];
    TTY::Print("mz_load: CS:IP=%p:%p->%p\n", header->initial_cs, header->initial_ip, lin_code);

    task.real.ss = start_segment + header->initial_ss;
    task.real.sp = header->initial_sp;
    task.real.cs = start_segment + header->initial_cs;
    task.real.ip = header->initial_ip;
}
