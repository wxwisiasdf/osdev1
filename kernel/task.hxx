#ifndef TASK_HXX
#define TASK_HXX 1

#include <cstdint>
#include "vendor.hxx"
#include "gdt.hxx"

#define MAX_TASKS (8)
#define MAX_STACK_SIZE (8192)

namespace IDT
{
    void SetTaskSegment(int seg);
}

namespace Task
{
    struct StackInfo
    {
        uint8_t stack[MAX_STACK_SIZE] ALIGN(4096) = {};
        bool is_used = false;
    };

    // Program segment prefix for each task
    struct PSP
    {
        uint16_t machine_code = 0xCD20; // INT 20h in machine code
        uint16_t top_memory;
        uint8_t reserved; // Usually 0
        union
        {
            uint8_t long_call[5];        // Long call to DOS func dispatcher, CP/M
            uint16_t bytes_avail_in_seg; // CP/M
        };
        uint32_t int22_term_addr;
        uint32_t int23_break_address;
        uint32_t int24_criterr_address;
        uint16_t parent_proc_segment;
        uint8_t file_handle[20];
        uint16_t segment_address; // Envrionemnt segment address or zero
        uint32_t ss_sp_on_entry;  // SS:SP on entry to last INT 21h func
        uint16_t array_size;
        uint32_t array_pointer;
        uint32_t previous_psp = 0xFFFFFFFF;
        uint8_t unused[20];
        uint8_t func_dispatch[3] = {0xCD, 0x21, 0xCB}; /* Call INT 21h */
        uint8_t unused2[9];
        uint8_t default_unopened_fcb1[36];
        uint8_t default_unopened_fcb2[20];
        uint8_t chars_in_tail;  // Count of characters in argument
        uint8_t arguments[127]; // Arguments following the command name, ending with a CR
    } PACKED;

    struct I286_TSS // 16-bit TSS
    {
        uint16_t link;
        uint16_t sp0;
        uint16_t ss0;
        uint16_t sp1;
        uint16_t ss1;
        uint16_t sp2;
        uint16_t ss2;
        uint16_t ip;
        uint16_t flags;
        uint16_t ax;
        uint16_t cx;
        uint16_t dx;
        uint16_t bx;
        uint16_t sp;
        uint16_t bp;
        uint16_t si;
        uint16_t di;
        uint16_t es;
        uint16_t cs;
        uint16_t ss;
        uint16_t ds;
        uint16_t ldtr;
    } PACKED ALIGN(4);

    struct I386_TSS // 32-bit TSS
    {
        uint16_t link;
        uint16_t reserved1;
        uint32_t esp0;
        uint16_t ss0;
        uint16_t reserved2;
        uint32_t esp1;
        uint16_t ss1;
        uint16_t reserved3;
        uint32_t esp2;
        uint16_t ss2;
        uint16_t reserved4;
        uint32_t cr3;
        uint32_t eip;
        uint32_t eflags;
        uint32_t eax;
        uint32_t ecx;
        uint32_t edx;
        uint32_t ebx;
        uint32_t esp;
        uint32_t ebp;
        uint32_t esi;
        uint32_t edi;
        uint16_t es;
        uint16_t reserved5;
        uint16_t cs;
        uint16_t reserved6;
        uint16_t ss;
        uint16_t reserved7;
        uint16_t ds;
        uint16_t reserved8;
        uint16_t fs;
        uint16_t reserved9;
        uint16_t gs;
        uint16_t reserved10;
        uint16_t ldtr;
        uint16_t reserved11;
        uint16_t reserved12;
        uint16_t iopb;
    } PACKED ALIGN(4);

    struct TSS
    {
        union
        {
            I286_TSS real;
            I386_TSS prot;
        };
        // --- Non x86 (shadow SP)
        uint32_t ssp;
        // --- OS dependant
        // TSS for this task
        uint16_t tss_segment;
        // LDT of this task
        uint16_t ldt_segment;
        struct
        {                          // Local entries
            LDT::Entry null;       // 0x00
            LDT::Entry kern_xcode; // 0x08
            LDT::Entry kern_xdata; // 0x10
            LDT::Entry user_xcode; // 0x18
            LDT::Entry user_xdata; // 0x20
            LDT::Entry stack;      // 0x28
        } local_entries;
        // Whetever this task is active
        bool is_active = false;
        // Is this a 286 task? or a 386 one?
        bool is_v86 = false;
        uint8_t io_bitmap[256 / 8];
    } ALIGN(4);

    Task::TSS &Schedule();
    void EnableSwitch();
    void DisableSwitch();
    bool CanSwitch();
    void Finish();
    struct ThreadSummary {
        unsigned int nActive;
        unsigned int nTotal;
    };
    ThreadSummary GetSummary();

    static inline void Switch()
    {
        if (!Task::CanSwitch())
            return;
        
        //asm("pushfl\r\n");
        auto &task = Task::Schedule();
        // Update interrupt entry accordingly
        IDT::SetTaskSegment(task.tss_segment);
        asm("int $0x84\r\n"
            /*"popfl\r\n"*/);
    }

    Task::StackInfo &GetStack();
    Task::TSS &Add(void (*eip)(), void *esp, bool v86);
    void Sleep(unsigned int usec);
}

#endif
