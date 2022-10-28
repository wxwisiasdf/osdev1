#include <optional>
import pic;
import uart;
import pci;
import usb;
import ps2;
import sb16;

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
#include "adlib.hxx"
#include "floppy.hxx"

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

extern "C" void Kernel_Init(unsigned long magic, uint8_t *addr)
{
    UART::com[0].emplace(0x3F8); // Initialize COM1

    static std::optional<TTY::Terminal> comTerm;
    comTerm.emplace();
    comTerm->AttachToKernel(comTerm.value());
    TTY::Print("Hello world\n");
    TTY::Print("Numbers %i,%u!!!\n", 69, 420);

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        return;

    for (auto *tag = (multiboot_tag *)(addr + 8);
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
        }
        break;
        default:
            break;
        }
    }

    GDT::Init();
    IDT_Init();
    GDT::SetupTSS();
    // Code should've jumped to Kernel_Main by now
}

extern "C" void Kernel_TaskV86Stub();
extern "C" void Kernel_EnterV86(uint32_t ss, uint32_t esp, uint32_t cs, uint32_t eip);
extern "C" uint8_t Kernel_V86StackTop;

// Main event handler, for keyboard and mouse
void Kernel_Main()
{
    // Sometimes kernel_main gets executed twice
    Task::DisableSwitch();
    PIC::Get().Remap(0xE8, 0xF0);
    Task::EnableSwitch();
    asm("sti"); // Always enable interrupts on the dummy task

    // Controllers
    static std::optional<EHCI::Device> usbDevice;
    usbDevice.emplace(PCI::Device{
        .bus = 0,
        .slot = 4,
        .func = 0,
    });
    static std::optional<PS2::Controller> ps2Controller;
    ps2Controller.emplace();

    // Input
    static std::optional<PS2::Keyboard> ps2Keyboard;
    ps2Keyboard.emplace(ps2Controller.value());
    static std::optional<PS2::Mouse> ps2Mouse;
    ps2Mouse.emplace(ps2Controller.value());

    // Audio
    static std::optional<AdLib::Device> adlibDevice;
    adlibDevice.emplace();
    static std::optional<SoundBlaster16> sb16;
    sb16.emplace(0x20, 2, SoundBlaster16::TransferMode::PLAYSOUND, SoundBlaster16::PCMMode::PCM_8);

    // Storage
    static std::optional<Floppy::Driver> floppyDriver;
    floppyDriver.emplace();

    static std::optional<ATAPI::Device> atapiDevices[2];
    atapiDevices[0].emplace(ATAPI::Device::Bus::PRIMARY);
    atapiDevices[1].emplace(ATAPI::Device::Bus::SECONDARY);

    static std::optional<ISO9660::Device> isoCdrom;
    isoCdrom.emplace(atapiDevices[1].value());
    static size_t offset = 0, padding = 0;
    isoCdrom->ReadFile("_bg.raw;1", [](void *data, size_t len) -> bool {
        TTY::Print("Reading %x bytes at %p\n", len, data);
        for (size_t i = 0; i < len / 3; i++)
        {
            const auto x = ((i + offset) == 0) ? 0 : (i + offset) % g_KFrameBuffer.width;
            const auto y = ((i + offset) == 0) ? 0 : (i + offset) / g_KFrameBuffer.width;
            const auto *p = reinterpret_cast<uint8_t *>(data);

            const auto r = p[i * 3 + 0 + padding] << 24;
            const auto g = p[i * 3 + 1 + padding] << 16;
            const auto b = p[i * 3 + 2 + padding] << 8;

            g_KFrameBuffer.PlotPixel(x, y, Color(r | g | b));
        }
        padding = 3 - len % 3;
        offset += len / 3;
        if (offset && offset / g_KFrameBuffer.width >= 180)
            return false;
        return true;
    });

    static std::optional<UI::Desktop> desktop;
    desktop.emplace();
    desktop->x = 0;
    desktop->y = 0;
    desktop->width = g_KFrameBuffer.width;
    desktop->height = g_KFrameBuffer.height;

    static std::optional<UI::Taskbar> taskbar;
    taskbar.emplace();
    taskbar->x = 0;
    taskbar->y = g_KFrameBuffer.height - 24;
    taskbar->width = g_KFrameBuffer.width;
    taskbar->height = 24;
    desktop->AddChild(taskbar.value());

    static std::optional<UI::Button> startBtn;
    startBtn.emplace();
    startBtn->x = 0;
    startBtn->y = 0;
    startBtn->width = 64;
    startBtn->height = 12;
    startBtn->SetText("Start");
    startBtn->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void
                         {
        static std::optional<UI::Taskbar> startMenu;
        if (startMenu.has_value())
        {
            startMenu.reset();
            return;
        }

        startMenu.emplace();
        startMenu->flex = UI::Widget::Flex::COLUMN;
        startMenu->width = 96;
        startMenu->height = 100;
        startMenu->x = 0;
        startMenu->y = taskbar->y - startMenu->height;
        desktop->AddChild(startMenu.value());

#if 0
        static std::optional<UI::Button> tourBtn;
        tourBtn.emplace();
        tourBtn->flex = startMenu->flex;
        tourBtn->x = tourBtn->y = 0;
        tourBtn->width = startMenu->width - 12;
        tourBtn->height = 12;
        tourBtn->SetText("Take a tour");
        tourBtn->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {

        });
        startMenu->AddChild(tourBtn.value());
#endif
    });
    taskbar->AddChild(startBtn.value());

    TTY::Print("System ready! Welcome to UDOS!\n");
    auto &uiMan = UI::Manager::Get();
    while (1)
    {
        char32_t ch = ps2Keyboard->GetKey();
        uiMan.Draw(desktop.value(), 0, 0);
        uiMan.CheckRedraw(desktop.value(), 0, 0);
        uiMan.CheckUpdate(desktop.value(), ps2Mouse->GetX(), ps2Mouse->GetY(),
                         ps2Mouse->buttons[0], ps2Mouse->buttons[1], ch);
        uiMan.Update();
        g_KFrameBuffer.MoveMouse(ps2Mouse->GetX(), ps2Mouse->GetY());
        Task::Switch();
    }
}
