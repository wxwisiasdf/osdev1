#include <optional>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include "atapi.hxx"
#include "iso9660.hxx"
#include "gdt.hxx"
#include "vga.hxx"
#include "video.hxx"
#include "multiboot2.h"
#include "task.hxx"
#include "ui.hxx"
#include "load.hxx"
#include "audio.hxx"
#include "alloc.hxx"

#include "pic.hxx"
#include "uart.hxx"
#include "pci.hxx"
#include "ps2.hxx"
#include "locale.hxx"
#include "filesys.hxx"
#include "drm.hxx"
#include "appkit.hxx"

struct Regs
{
    union
    {
        struct
        {
            uint16_t sp;
            uint16_t ax;
            uint16_t bx;
            uint16_t cx;
            uint16_t dx;
            uint16_t bp;
            uint16_t cs;
            uint16_t ds;
            uint16_t ss;
        } r16;
        struct
        {
            uint32_t esp;
            uint32_t eax;
            uint32_t ebx;
            uint32_t ecx;
            uint32_t edx;
            uint32_t gs;
            uint32_t fs;
        } r32;
    };
};

struct FCB_Date
{
    uint16_t day : 5;
    uint16_t month : 4;
    uint16_t year : 7;
} PACKED;

struct FCB_Time
{
    uint16_t secs : 5;
    uint16_t minutes : 6;
    uint16_t hours : 5;
} PACKED;

struct FCB_ExtendedPrefix
{
    uint8_t is_extended; // 0xFF if extended
    uint8_t reserved[5];
    uint8_t attribute; // If extended FCB
} PACKED;

struct FCB
{
    FCB_ExtendedPrefix prefix;
    uint8_t drive_num;
    uint8_t filename[8]; // Left justified with spaces
    uint8_t extension[3];
    uint16_t curr_blocknum; // Current block number
    uint16_t lrecl_size;    // Logical record size in bytes
    uint32_t file_size;     // File size in bytes
    union
    {
        FCB_Date update_date;
        FCB_Date creation_date;
    };
    FCB_Time last_write_time;
    uint8_t reserved2[8];
    uint32_t device_header;   // Address of device header
    uint8_t rel_recnum;       // Relative record number withing current BLOCK
    uint32_t rel_recnum_file; // High bit ommited if reclenght is 64-bytes
};

extern uint8_t text_start, text_end;
extern uint8_t rodata_start, rodata_end;
extern uint8_t data_start, data_end;

static bool kernelInitLock = false;
static bool hasGraphics = false;
extern "C" void Kernel_Init(unsigned long magic, uint8_t *addr)
{
    if(kernelInitLock)
        while(1)
            Task::Switch();
    kernelInitLock = true;

    HimemAlloc::InitManager(HimemAlloc::Manager::GetDefault());
    static uint8_t memHeap[65536 * 8];
    HimemAlloc::AddRegion(HimemAlloc::Manager::GetDefault(), (void *)memHeap, sizeof(memHeap) - 1);

    /*for (auto *tag = (multiboot_tag *)(addr + 8);
            tag->type != MULTIBOOT_TAG_TYPE_END;
            tag = reinterpret_cast<decltype(tag)>((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        switch (tag->type)
        {
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
        {
            new (&g_KFrameBuffer) Framebuffer(*(multiboot_tag_framebuffer *)tag);
            auto &uiMan = UI::Manager::Get();
            new (&uiMan) UI::Manager(g_KFrameBuffer);
            hasGraphics = true;
        }
        break;
        default:
            break;
        }
    }*/

    UART::com[0].emplace(0x3F8); // Initialize COM1
    static std::optional<TTY::Terminal> comTerm;
    comTerm.emplace();
    comTerm->AttachToKernel(comTerm.value());

    if(!hasGraphics)
    {
        static std::optional<VGATerm> vgaTerm;
        vgaTerm.emplace();
        vgaTerm->AttachToKernel(vgaTerm.value());
    }

    TTY::Print("Hello world\n");
    TTY::Print("Numbers %i,%u!!!\n", 69, 420);

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        return;

    GDT::Init();
    IDT::Init();
    GDT::SetupTSS();
    // Code should've jumped to Kernel_Main by now
}

extern "C" void Kernel_TaskV86Stub();
extern "C" void Kernel_EnterV86(uint32_t ss, uint32_t esp, uint32_t cs, uint32_t eip);
extern "C" uint8_t Kernel_V86StackTop;

std::optional<UI::Desktop> g_Desktop;

#if 0
static inline std::vector<std::string> SplitStrings(const std::string& s, char seperator)
{
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;
    while((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring(s.substr(prev_pos, pos-prev_pos));
        output.push_back(substring);
        prev_pos = ++pos;
    }
    output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word
    return output;
}
#endif 

std::optional<PS2::Controller> ps2Controller;
std::optional<PS2::Keyboard> ps2Keyboard;
std::optional<PS2::Mouse> ps2Mouse;
std::optional<ATAPI::Device> atapiDevices[2];
std::optional<ISO9660::Device> isoCdrom;
// Main event handler, for keyboard and mouse
static bool kernelMainLock = false;
void Kernel_Main()
{
    if(kernelMainLock)
        while(1)
            Task::Switch();
    kernelMainLock = true;

    // Sometimes kernel_main gets executed twice
    Task::DisableSwitch();
    PIC::Get().Remap(0xE8, 0xF0);
    Task::EnableSwitch();
    asm("sti"); // Always enable interrupts on the dummy task

    TTY::Print("Initializing early boot\n");
    TTY::Print("Enabled interrupts and dying softly\n");

    // Controllers
    TTY::Print("Initializing PS2\n");
    ps2Controller.emplace();

    // Input
    TTY::Print("Initializing PS2 keyboard\n");
    ps2Keyboard.emplace(ps2Controller.value());

    TTY::Print("Initializing the fucking PS2 mouse\n");
    ps2Mouse.emplace(ps2Controller.value());

    // Storage
    TTY::Print("Initializing ATAPI\n");
    atapiDevices[0].emplace(ATAPI::Device::Bus::PRIMARY);
    atapiDevices[1].emplace(ATAPI::Device::Bus::SECONDARY);

    TTY::Print("Initializing ISO CD-ROM\n");
    isoCdrom.emplace(atapiDevices[1].value());

#if 0
    static std::string menuConfig;
    auto r = isoCdrom->ReadFile("menu.cfg;1", [](void *data, size_t len) -> bool
    {
        TTY::Print("Taskbar config: %s\n", data);
        menuConfig += (const char *)data;
        return true;
    });
    TTY::Print("Taskbar config (%u): %s\n", menuConfig.size(), menuConfig.c_str());
    const auto lines = SplitStrings(menuConfig, '\n');
    for(const auto& line : lines)
    {
        const auto data = SplitStrings(line, ';');
        if(data.size() <= 3)
        {
            TTY::Print("Menu config has malformed entry %s\n", line.c_str());
            continue;
        }

        const auto& name = data[0];
        const auto& desc = data[1];
        const auto& filename = data[2];
    }
#endif

    TTY::Print("Finished!\n");
    TTY::Print("System ready! Welcome to UDOS!\n");
    HimemAlloc::Print(HimemAlloc::Manager::GetDefault());
    // Filesys::Init();
    while (1)
    {
        Task::Switch();
    }
}
