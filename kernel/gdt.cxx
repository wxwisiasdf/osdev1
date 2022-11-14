#include <cstddef>
#include "gdt.hxx"
#include "task.hxx"
#include "tty.hxx"
#include "vendor.hxx"

extern uint8_t g_KernStackTop;
extern uint8_t text_start, text_end;
extern uint8_t data_start, data_end;
extern uint8_t rodata_start, rodata_end;
extern void Kernel_Main();
static struct
{
    struct
    {
        GDT::Entry null = {};
        GDT::Entry system[MAX_GDT_ENTRIES] = {};
    } entries;
    GDT::Desc desc = {};
} gdt;

void GDT::Reload()
{
    asm volatile("\tlgdt %0\r\n"
                 :
                 : "m"(gdt.desc));
    asm goto("\tjmp %0,%1\r\n"
             :
             : "i"(GDT::KERNEL_XCODE)
             :
             : reload_segments);
reload_segments:
    asm volatile("\tmov %0,%%ds\r\n"
                 "\tmov %0,%%es\r\n"
                 "\tmov %0,%%fs\r\n"
                 "\tmov %0,%%gs\r\n"
                 "\tmov %0,%%ss\r\n"
                 :
                 : "r"(GDT::KERNEL_XDATA)
                 :);
}

GDT::Entry &GDT::AllocateEntry()
{
    for (size_t i = 0; i < ARRAY_SIZE(gdt.entries.system); i++)
    {
        auto &entry = gdt.entries.system[i];
        if (!entry.access.present)
        {
            entry.access.present = 1;
            return entry;
        }
    }
    __builtin_unreachable();
}

int GDT::GetEntrySegment(const GDT::Entry &entry)
{
    return ((uintptr_t)&entry - (uintptr_t)&gdt.entries);
}

void GDT::Init()
{
    gdt.desc.size = sizeof(gdt.entries) - 1;
    // TODO: Aliasing violation
    gdt.desc.offset = (uintptr_t)&gdt.entries;

    auto *entry = &gdt.entries.system[0];
    entry[0].SetBase(0); // Kernel 32 bit code, 0x08
    entry[0].SetLimit(0xFFFFFFFF);
    entry[0].flags.size = 1;
    entry[0].access.present = 1;
    entry[0].access.privilege = 0;
    entry[0].access.always_1 = 1;
    entry[0].access.executable = 1;
    entry[0].access.read_write = 0;

    entry[1].SetBase(0); // Kernel 32 bit data, 0x10
    entry[1].SetLimit(0xFFFFFFFF);
    entry[1].flags.size = 1;
    entry[1].access.present = 1;
    entry[1].access.privilege = 0;
    entry[1].access.always_1 = 1;
    entry[1].access.executable = 0;
    entry[1].access.read_write = 1;
    GDT::Reload(); // Reload the GDT so the IDT can be properly setup
}

void GDT::SetupTSS()
{
    // PC of the first task is overriden with current pc ;)
    auto &task = Task::Add(&Kernel_Main, &g_KernStackTop, false);
    
    static uint8_t dummyTaskStack[1024];
    Task::Add([]() -> void {
        while (1)
        {
            //TTY::Print("A");
            Task::Switch();
        }
    }, &dummyTaskStack[sizeof(dummyTaskStack) - 1], false);

    GDT::Reload();                 // Apply visable changes
    asm volatile("\tlldt %%ax\r\n" // Load our local LDT
                 :
                 : "a"(task.prot.ldtr)
                 :);
    asm volatile("\tltr %%ax\r\n" // Load TSS
                 :
                 : "a"(task.tss_segment)
                 :);
    // What will happen now is:
    // - The jump will save the EIP into the task 1
    // - We will jump to the new kernel task (selected by the segment)
    // - :ip in this jump is ignored
    asm volatile("\tljmp %0,$0\r\n"
                 :
                 : "i"(8 * 6)
                 :);
    // We will call directly the Kernel_Main
    asm volatile("\tmov %1,%%eax\r\n"
                 "\tmov %0,%%esp\r\n"
                 "\tjmp *%%eax\r\n"
                 :
                 : "i"(&g_KernStackTop), "l"(&Kernel_Main)
                 :);
}

GDT::Entry &GDT::GetEntry(unsigned segment)
{
    segment >>= 3; // Divide by 8
    if (!segment)
        return gdt.entries.null;
    return gdt.entries.system[segment - 1];
}

static struct
{
    IDT_Entry entries[256] = {};
    IDT_Desc desc = {
        .size = sizeof(entries) - 1,
        // TODO: Aliasing violation
        .offset = (uintptr_t)&entries[0]};
} idt;

#define INT_STUB(x)                            \
    extern "C" void Int##x##h_AsmStub();       \
    extern "C" void Int##x##h_Handler()        \
    {                                          \
        TTY::Print("Unhandled int " #x "h\n"); \
        while (1);                             \
    }

extern "C" void Int00h_AsmStub();
extern "C" void Int01h_AsmStub();
extern "C" void Int02h_AsmStub();
extern "C" void Int03h_AsmStub();
extern "C" void Int04h_AsmStub();
extern "C" void Int05h_AsmStub();
extern "C" void Int06h_AsmStub();
extern "C" void Int07h_AsmStub();
extern "C" void Int08h_AsmStub();
extern "C" void Int09h_AsmStub();
extern "C" void Int0Ah_AsmStub();
extern "C" void Int0Bh_AsmStub();
extern "C" void Int0Ch_AsmStub();
extern "C" void Int0Dh_AsmStub();
extern "C" void Int0Eh_AsmStub();
extern "C" void Int0Fh_AsmStub();
extern "C" void Int10h_AsmStub();
extern "C" void Int11h_AsmStub();
extern "C" void Int12h_AsmStub();
extern "C" void Int13h_AsmStub();
extern "C" void Int14h_AsmStub();
extern "C" void Int15h_AsmStub();
extern "C" void Int16h_AsmStub();
extern "C" void Int17h_AsmStub();
extern "C" void Int18h_AsmStub();
extern "C" void Int19h_AsmStub();
extern "C" void Int1Ah_AsmStub();
extern "C" void Int1Bh_AsmStub();
extern "C" void Int1Ch_AsmStub();
extern "C" void Int1Dh_AsmStub();
extern "C" void Int1Eh_AsmStub();
extern "C" void Int1Fh_AsmStub();

struct V86_IntContext
{
    uint32_t eip;    // 0
    uint32_t cs;     // 4
    uint32_t eflags; // 8
    uint32_t esp;    // 12
    uint32_t ss;     // 16
    uint32_t es;     // 20
    uint32_t ds;     // 24
    uint32_t fs;     // 28
    uint32_t gs;     // 32
};

#include "ui.hxx"
extern "C" void IntException_Handler(uint32_t irq, V86_IntContext &ctx)
{
    static const char *errorNames[32] = {
        "Divide-by-zero Error",           // 0x00
        "Debug",                          // 0x01
        "Non-maskable Interrupt",         // 0x02
        "Breakpoint",                     // 0x03
        "Overflow",                       // 0x04
        "Bound Range Exceeded",           // 0x05
        "Invalid Opcode",                 // 0x06
        "Device Not Available",           // 0x07
        "Double Fault",                   // 0x08
        "Coprocessor Segment Overrun",    // 0x09
        "Invalid TSS",                    // 0x0A
        "Segment Not Present",            // 0x0B
        "Stack-Segment Fault",            // 0x0C
        "General Protection Fault",       // 0x0D
        "Page Fault",                     // 0x0E
        "Reserved",                       // 0x0F
        "x87 Floating-Point Exception",   // 0x10
        "Alignment Check",                // 0x11
        "Machine Check",                  // 0x12
        "SIMD Floating-Point Exception",  // 0x13
        "Virtualization Exception",       // 0x14
        "Control Protection Exception",   // 0x15
        "Reserved",                       // 0x16
        "Reserved",                       // 0x17
        "Reserved",                       // 0x18
        "Reserved",                       // 0x19
        "Reserved",                       // 0x1A
        "Reserved",                       // 0x1B
        "Hypervisor Injection Exception", // 0x1C
        "VMM Communication Exception",    // 0x1D
        "Security Exception",             // 0x1E
        "Reserved"                        // 0x1F
    };

    if (irq != 0x08)
    {
        UI::Manager::Get().ErrorMsg(*"CPU Error", *errorNames[irq % 32]);
        UI::Manager::Get().Draw(*UI::Manager::Get().GetRoot(), 0, 0);
    }

    TTY::Print("%s CS:IP=%p:%p\n", errorNames[irq % 32], ctx.cs, ctx.eip);
    ctx.cs = GDT::USER_XCODE | GDT::LDT_SEGMENT;
    ctx.ds = GDT::USER_XDATA | GDT::LDT_SEGMENT;
    ctx.ss = ctx.ds;
    ctx.es = ctx.ds;
    ctx.fs = ctx.ds;
    ctx.gs = ctx.ds;
    while (1)
        ;
}

INT_STUB(20);

extern "C" void Int21h_AsmStub();
extern "C" void Int21h_Handler()
{
    TTY::Print("int 21h\n");
}

INT_STUB(22);
INT_STUB(23);
INT_STUB(24);
INT_STUB(25);
INT_STUB(26);
INT_STUB(27);
INT_STUB(28);
INT_STUB(29);
INT_STUB(2A);
INT_STUB(2B);
INT_STUB(2C);
INT_STUB(2D);
INT_STUB(2E);
INT_STUB(2F);
INT_STUB(30);
INT_STUB(31);
INT_STUB(32);
INT_STUB(33);
INT_STUB(34);
INT_STUB(35);
INT_STUB(36);
INT_STUB(37);
INT_STUB(38);
INT_STUB(39);
INT_STUB(3A);
INT_STUB(3B);
INT_STUB(3C);
INT_STUB(3D);
INT_STUB(3E);
INT_STUB(3F);
INT_STUB(40);
INT_STUB(41);
INT_STUB(42);
INT_STUB(43);
INT_STUB(44);
INT_STUB(45);
INT_STUB(46);
INT_STUB(47);
INT_STUB(48);
INT_STUB(49);
INT_STUB(4A);
INT_STUB(4B);
INT_STUB(4C);
INT_STUB(4D);
INT_STUB(4E);
INT_STUB(4F);
INT_STUB(50);
INT_STUB(51);
INT_STUB(52);
INT_STUB(53);
INT_STUB(54);
INT_STUB(55);
INT_STUB(56);
INT_STUB(57);
INT_STUB(58);
INT_STUB(59);
INT_STUB(5A);
INT_STUB(5B);
INT_STUB(5C);
INT_STUB(5D);
INT_STUB(5E);
INT_STUB(5F);
INT_STUB(60);
INT_STUB(61);
INT_STUB(62);
INT_STUB(63);
INT_STUB(64);
INT_STUB(65);
INT_STUB(66);
INT_STUB(67);
INT_STUB(68);
INT_STUB(69);
INT_STUB(6A);
INT_STUB(6B);
INT_STUB(6C);
INT_STUB(6D);
INT_STUB(6E);
INT_STUB(6F);
INT_STUB(70);
INT_STUB(71);
INT_STUB(72);
INT_STUB(73);
INT_STUB(74);
INT_STUB(75);
INT_STUB(76);
INT_STUB(77);
INT_STUB(78);
INT_STUB(79);
INT_STUB(7A);
INT_STUB(7B);
INT_STUB(7C);
INT_STUB(7D);
INT_STUB(7E);
INT_STUB(7F);
INT_STUB(80);
INT_STUB(81);
INT_STUB(82);
INT_STUB(83);
// INT_STUB(84); -- not required
INT_STUB(85);
INT_STUB(86);
INT_STUB(87);
INT_STUB(88);
INT_STUB(89);
INT_STUB(8A);
INT_STUB(8B);
INT_STUB(8C);
INT_STUB(8D);
INT_STUB(8E);
INT_STUB(8F);
INT_STUB(90);
INT_STUB(91);
INT_STUB(92);
INT_STUB(93);
INT_STUB(94);
INT_STUB(95);
INT_STUB(96);
INT_STUB(97);
INT_STUB(98);
INT_STUB(99);
INT_STUB(9A);
INT_STUB(9B);
INT_STUB(9C);
INT_STUB(9D);
INT_STUB(9E);
INT_STUB(9F);
INT_STUB(A0);
INT_STUB(A1);
INT_STUB(A2);
INT_STUB(A3);
INT_STUB(A4);
INT_STUB(A5);
INT_STUB(A6);
INT_STUB(A7);
INT_STUB(A8);
INT_STUB(A9);
INT_STUB(AA);
INT_STUB(AB);
INT_STUB(AC);
INT_STUB(AD);
INT_STUB(AE);
INT_STUB(AF);
INT_STUB(B0);
INT_STUB(B1);
INT_STUB(B2);
INT_STUB(B3);
INT_STUB(B4);
INT_STUB(B5);
INT_STUB(B6);
INT_STUB(B7);
INT_STUB(B8);
INT_STUB(B9);
INT_STUB(BA);
INT_STUB(BB);
INT_STUB(BC);
INT_STUB(BD);
INT_STUB(BE);
INT_STUB(BF);
INT_STUB(C0);
INT_STUB(C1);
INT_STUB(C2);
INT_STUB(C3);
INT_STUB(C4);
INT_STUB(C5);
INT_STUB(C6);
INT_STUB(C7);
INT_STUB(C8);
INT_STUB(C9);
INT_STUB(CA);
INT_STUB(CB);
INT_STUB(CC);
INT_STUB(CD);
INT_STUB(CE);
INT_STUB(CF);
INT_STUB(D0);
INT_STUB(D1);
INT_STUB(D2);
INT_STUB(D3);
INT_STUB(D4);
INT_STUB(D5);
INT_STUB(D6);
INT_STUB(D7);
INT_STUB(D8);
INT_STUB(D9);
INT_STUB(DA);
INT_STUB(DB);
INT_STUB(DC);
INT_STUB(DD);
INT_STUB(DE);
INT_STUB(DF);
INT_STUB(E0);
INT_STUB(E1);
INT_STUB(E2);
INT_STUB(E3);
INT_STUB(E4);
INT_STUB(E5);
INT_STUB(E6);
INT_STUB(E7);
// INT_STUB(E8); -- Implemented by pit.cxx
extern "C" void IntE8h_AsmStub();
extern "C" void IntE8h_Handler();
// INT_STUB(E9); -- Implemented by ps2.cxx
extern "C" void IntE9h_AsmStub();
extern "C" void IntE9h_Handler();
INT_STUB(EA);
INT_STUB(EB);
INT_STUB(EC);
// INT_STUB(ED); -- Implemented by sb16.cxx
extern "C" void IntEDh_AsmStub();
extern "C" void IntEDh_Handler();
INT_STUB(EE);
INT_STUB(EF);
INT_STUB(F0);
INT_STUB(F1);
INT_STUB(F2);
INT_STUB(F3);
// INT_STUB(F4); -- Implemented by ps2.cxx
extern "C" void IntF4h_AsmStub();
extern "C" void IntF4h_Handler();
// INT_STUB(F5); -- Implemented by pic.cxx
extern "C" void IntF5h_AsmStub();
extern "C" void IntF5h_Handler();
// INT_STUB(F6); -- Implemented by atapi.cxx
extern "C" void IntF6h_AsmStub();
extern "C" void IntF6h_Handler();
// INT_STUB(F7); -- Implemented by atapi.cxx
extern "C" void IntF7h_AsmStub();
extern "C" void IntF7h_Handler();
INT_STUB(F8);
INT_STUB(F9);
INT_STUB(FA);
INT_STUB(FB);
INT_STUB(FC);
INT_STUB(FD);
INT_STUB(FE);
INT_STUB(FF);

#define SET_ASM_STUB(x) \
    idt.entries[0x##x].set_offset((uintptr_t)&Int##x##h_AsmStub);

void IDT_Init()
{
    // Exceptions
    for (size_t i = 0x00; i < 0x20; i++)
    {
        idt.entries[i].gate_type = IDT_Entry::GATE_32TRAP;
        idt.entries[i].seg_sel = GDT::KERNEL_XCODE;
        idt.entries[i].present = 1;
    }
    SET_ASM_STUB(00);
    SET_ASM_STUB(01);
    SET_ASM_STUB(02);
    SET_ASM_STUB(03);
    SET_ASM_STUB(04);
    SET_ASM_STUB(05);
    SET_ASM_STUB(06);
    SET_ASM_STUB(07);
    SET_ASM_STUB(08);
    SET_ASM_STUB(09);
    SET_ASM_STUB(0A);
    SET_ASM_STUB(0B);
    SET_ASM_STUB(0C);
    SET_ASM_STUB(0D);
    SET_ASM_STUB(0E);
    SET_ASM_STUB(0F);
    SET_ASM_STUB(10);
    SET_ASM_STUB(11);
    SET_ASM_STUB(12);
    SET_ASM_STUB(13);
    SET_ASM_STUB(14);
    SET_ASM_STUB(15);
    SET_ASM_STUB(16);
    SET_ASM_STUB(17);
    SET_ASM_STUB(18);
    SET_ASM_STUB(19);
    SET_ASM_STUB(1A);
    SET_ASM_STUB(1B);
    SET_ASM_STUB(1C);
    SET_ASM_STUB(1D);
    SET_ASM_STUB(1E);
    SET_ASM_STUB(1F);

    // IRQs of devices or syscalls, all present w/ stubs
    for (unsigned int i = 0x20; i < 0xFF; i++)
    {
        idt.entries[i].gate_type = IDT_Entry::GATE_32INT;
        idt.entries[i].seg_sel = GDT::KERNEL_XCODE;
        idt.entries[i].present = 1;
    }

    SET_ASM_STUB(20);
    SET_ASM_STUB(21);
    SET_ASM_STUB(22);
    SET_ASM_STUB(23);
    SET_ASM_STUB(24);
    SET_ASM_STUB(25);
    SET_ASM_STUB(26);
    SET_ASM_STUB(27);
    SET_ASM_STUB(28);
    SET_ASM_STUB(29);
    SET_ASM_STUB(2A);
    SET_ASM_STUB(2B);
    SET_ASM_STUB(2C);
    SET_ASM_STUB(2D);
    SET_ASM_STUB(2E);
    SET_ASM_STUB(2F);
    SET_ASM_STUB(30);
    SET_ASM_STUB(31);
    SET_ASM_STUB(32);
    SET_ASM_STUB(33);
    SET_ASM_STUB(34);
    SET_ASM_STUB(35);
    SET_ASM_STUB(36);
    SET_ASM_STUB(37);
    SET_ASM_STUB(38);
    SET_ASM_STUB(39);
    SET_ASM_STUB(3A);
    SET_ASM_STUB(3B);
    SET_ASM_STUB(3C);
    SET_ASM_STUB(3D);
    SET_ASM_STUB(3E);
    SET_ASM_STUB(3F);
    SET_ASM_STUB(40);
    SET_ASM_STUB(41);
    SET_ASM_STUB(42);
    SET_ASM_STUB(43);
    SET_ASM_STUB(44);
    SET_ASM_STUB(45);
    SET_ASM_STUB(46);
    SET_ASM_STUB(47);
    SET_ASM_STUB(48);
    SET_ASM_STUB(49);
    SET_ASM_STUB(4A);
    SET_ASM_STUB(4B);
    SET_ASM_STUB(4C);
    SET_ASM_STUB(4D);
    SET_ASM_STUB(4E);
    SET_ASM_STUB(4F);
    SET_ASM_STUB(50);
    SET_ASM_STUB(51);
    SET_ASM_STUB(52);
    SET_ASM_STUB(53);
    SET_ASM_STUB(54);
    SET_ASM_STUB(55);
    SET_ASM_STUB(56);
    SET_ASM_STUB(57);
    SET_ASM_STUB(58);
    SET_ASM_STUB(59);
    SET_ASM_STUB(5A);
    SET_ASM_STUB(5B);
    SET_ASM_STUB(5C);
    SET_ASM_STUB(5D);
    SET_ASM_STUB(5E);
    SET_ASM_STUB(5F);
    SET_ASM_STUB(50);
    SET_ASM_STUB(51);
    SET_ASM_STUB(52);
    SET_ASM_STUB(53);
    SET_ASM_STUB(54);
    SET_ASM_STUB(55);
    SET_ASM_STUB(56);
    SET_ASM_STUB(57);
    SET_ASM_STUB(58);
    SET_ASM_STUB(59);
    SET_ASM_STUB(5A);
    SET_ASM_STUB(5B);
    SET_ASM_STUB(5C);
    SET_ASM_STUB(5D);
    SET_ASM_STUB(5E);
    SET_ASM_STUB(5F);
    SET_ASM_STUB(60);
    SET_ASM_STUB(61);
    SET_ASM_STUB(62);
    SET_ASM_STUB(63);
    SET_ASM_STUB(64);
    SET_ASM_STUB(65);
    SET_ASM_STUB(66);
    SET_ASM_STUB(67);
    SET_ASM_STUB(68);
    SET_ASM_STUB(69);
    SET_ASM_STUB(6A);
    SET_ASM_STUB(6B);
    SET_ASM_STUB(6C);
    SET_ASM_STUB(6D);
    SET_ASM_STUB(6E);
    SET_ASM_STUB(6F);
    SET_ASM_STUB(70);
    SET_ASM_STUB(71);
    SET_ASM_STUB(72);
    SET_ASM_STUB(73);
    SET_ASM_STUB(74);
    SET_ASM_STUB(75);
    SET_ASM_STUB(76);
    SET_ASM_STUB(77);
    SET_ASM_STUB(78);
    SET_ASM_STUB(79);
    SET_ASM_STUB(7A);
    SET_ASM_STUB(7B);
    SET_ASM_STUB(7C);
    SET_ASM_STUB(7D);
    SET_ASM_STUB(7E);
    SET_ASM_STUB(7F);

    SET_ASM_STUB(E8); // PIT - Master PIC IRQs
    SET_ASM_STUB(E9); // Keyboard
    SET_ASM_STUB(EA); // Cascade
    SET_ASM_STUB(EB); // COM1
    SET_ASM_STUB(EC); // COM2
    SET_ASM_STUB(ED); // LPT2
    SET_ASM_STUB(EE); // Floppy
    SET_ASM_STUB(EF); // LPT1
    SET_ASM_STUB(F0); // CMOS
    SET_ASM_STUB(F1); // Free - Slave PIC IRQs
    SET_ASM_STUB(F2); // Free
    SET_ASM_STUB(F3); // Free
    SET_ASM_STUB(F4); // PS2 mouse
    SET_ASM_STUB(F5); // FPU coprocessor
    SET_ASM_STUB(F6); // Primary ATA disk
    SET_ASM_STUB(F7); // Secondary ATA disk
    SET_ASM_STUB(F8);
    SET_ASM_STUB(F9);
    SET_ASM_STUB(FA);
    SET_ASM_STUB(FB);
    SET_ASM_STUB(FC);
    SET_ASM_STUB(FD);
    SET_ASM_STUB(FE);
    SET_ASM_STUB(FF);

    // Task switch interrupt, $0x84 doesn't seem to be used by any
    // DOS program so this should be fine-ish
    idt.entries[0x84].gate_type = IDT_Entry::GATE_TASK;
    idt.entries[0x84].present = 1;
    idt.entries[0x84].set_offset(0);

    asm volatile("\tlidt %0\r\n"
                 :
                 : "m"(idt.desc));
}

/// @brief Set the segment selector for the task interrupt
/// @param seg Segment to taskswitch to
void IDT::SetTaskSegment(int seg)
{
    idt.entries[0x84].seg_sel = seg;
}
