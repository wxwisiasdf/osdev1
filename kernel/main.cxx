#include <optional>
import pic;
import uart;
import pci;
import usb;
import ps2;
import sb16;
import locale;

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
#include "alloc.hxx"
#include "vfs.hxx"

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

    HimemAlloc::InitManager(HimemAlloc::Manager::GetDefault());
    HimemAlloc::AddRegion(HimemAlloc::Manager::GetDefault(), (void *)0x1000000, 65536 * 4);

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

std::optional<UI::Desktop> g_Desktop;

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

    // 0x1000000;
    g_Desktop.emplace();
    g_Desktop->x = 0;
    g_Desktop->y = 0;
    g_Desktop->width = g_KFrameBuffer.width;
    g_Desktop->height = g_KFrameBuffer.height;

    HimemAlloc::Print(HimemAlloc::Manager::GetDefault());

    auto& taskbar = g_Desktop->AddChild<UI::Taskbar>();
    taskbar.x = 0;
    taskbar.y = g_KFrameBuffer.height - 20;
    taskbar.width = g_KFrameBuffer.width;
    taskbar.height = 20;

    auto& startBtn = taskbar.AddChild<UI::Button>();
    startBtn.x = 0;
    startBtn.y = 0;
    startBtn.width = 64;
    startBtn.height = 12;
    startBtn.SetText("Start");
    startBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void
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
        startMenu->y = (g_KFrameBuffer.height - 20) - startMenu->height;
        g_Desktop->AddChildDirect(startMenu.value());

        auto& runBtn = startMenu->AddChild<UI::Button>();
        runBtn.flex = startMenu->flex;
        runBtn.x = runBtn.y = 0;
        runBtn.width = startMenu->width - 12;
        runBtn.height = 12;
        runBtn.SetText("Run");
        runBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
            auto& runWin = g_Desktop->AddChild<UI::Window>();;
            runWin.SetText("Run a program");
            runWin.x = runWin.y = 0;
            runWin.width = 200;
            runWin.height = 64;
            runWin.Decorate();

            static std::optional<UI::Textbox> filepathTextbox;
            filepathTextbox.emplace();
            filepathTextbox->x = filepathTextbox->y = 0;
            filepathTextbox->width = runWin.width - 12;
            filepathTextbox->height = 12;
            filepathTextbox->SetText("");
            filepathTextbox->SetTooltipText("The filepath for executing the program");
            filepathTextbox->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {

            });
            runWin.AddChildDirect(filepathTextbox.value());

            auto& runBtn = runWin.AddChild<UI::Button>();
            runBtn.x = 0;
            runBtn.y = 16;
            runBtn.width = runWin.width - 12;
            runBtn.height = 12;
            runBtn.SetText("Run");
            runBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
                Task::Add([]() -> void  {
                    static char tmpbuf[100];
                    for(size_t i = 0; i < 100; i++)
                        tmpbuf[i] = Locale::Convert<Locale::Charset::ASCII, Locale::Charset::NATIVE>((char)filepathTextbox->textBuffer[i]);
                    
                    static size_t offset = 0;
                    auto r = isoCdrom->ReadFile(tmpbuf, [](void *data, size_t len) -> bool {
                        TTY::Print("Reading %x bytes at %p\n", len, data);
                        auto *dest = (uint8_t *)0x1000000 + offset;
                        const auto *src = (const uint8_t *)data;
                        for (size_t i = 0; i < len; i++)
                            dest[i] = src[i];
                        return true;
                    });

                    if (!r)
                    {
                        auto& errorWin = g_Desktop->AddChild<UI::Window>();
                        errorWin.SetText("Error notFound");
                        errorWin.x = errorWin.y = 0;
                        errorWin.width = 150;
                        errorWin.height = 48;
                        errorWin.Decorate();
                        while (!errorWin.isClosed)
                            Task::Switch();
                    }
                    else
                    {
                        Task::Add((void (*)())0x1000000, nullptr, false);
                    }
                }, nullptr, false);
            });
        });

        auto& systemBtn = startMenu->AddChild<UI::Button>();
        systemBtn.flex = startMenu->flex;
        systemBtn.x = systemBtn.y = 0;
        systemBtn.width = startMenu->width - 12;
        systemBtn.height = 12;
        systemBtn.SetText("System");
        systemBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
            Task::Add([]() {
                auto& systemWin = g_Desktop->AddChild<UI::Window>();
                systemWin.SetText("System information");
                systemWin.x = 0;
                systemWin.y = 0;
                systemWin.width = 200;
                systemWin.height = 150;
                systemWin.Decorate();

                auto& infoTextbox = systemWin.AddChild<UI::Textbox>();
                infoTextbox.SetText("...");
                infoTextbox.y = infoTextbox.x = 0;
                infoTextbox.width = systemWin.width - 24;
                infoTextbox.height = systemWin.height - 32;
                infoTextbox.OnUpdate = [](UI::Widget& w) {
                    auto& o = static_cast<UI::Textbox&>(w);
                    auto fmtPrint = [](const char *fmt, ...) {
                        va_list args;
                        va_start(args, fmt);
                        static char tmpbuf[300] = {};
                        TTY::Print_1(tmpbuf, sizeof(tmpbuf), fmt, args);
                        va_end(args);
                        return tmpbuf;
                    };
                    auto ts = Task::GetSummary();
                    auto& mm = HimemAlloc::Manager::GetDefault();
                    auto ms = HimemAlloc::GetInfo(mm);
                    o.SetText(fmtPrint("ATasks: %u\nTTasks: %u\nMRegions: %u\nMTotal: %uB\nMFree: %uB\nMUsed: %uB", ts.nActive, ts.nTotal, mm.max_regions, ms.total, ms.free, ms.total - ms.free));
                };
                infoTextbox.OnUpdate(infoTextbox);

                while (!systemWin.isClosed)
                    Task::Switch();
            }, nullptr, false);
        });

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
        startMenu->AddChildDirect(tourBtn.value());
#endif
    });

    TTY::Print("System ready! Welcome to UDOS!\n");
    HimemAlloc::Print(HimemAlloc::Manager::GetDefault());

    // Filesystem::Init();

    auto &uiMan = UI::Manager::Get();
    while (1)
    {
        char32_t ch = ps2Keyboard->GetKey();
        uiMan.Draw(g_Desktop.value(), 0, 0);
        uiMan.CheckRedraw(g_Desktop.value(), 0, 0);
        uiMan.CheckUpdate(g_Desktop.value(), ps2Mouse->GetX(), ps2Mouse->GetY(),
                         ps2Mouse->buttons[0], ps2Mouse->buttons[1], ch);
        uiMan.Update();
        g_KFrameBuffer.MoveMouse(ps2Mouse->GetX(), ps2Mouse->GetY());
        Task::Switch();
    }
}

#if 0
iso9660: Entry Makefile.;1 (B)
iso9660: Entry bg00.png;1 (A)
iso9660: Entry bg01.png;1 (A)
iso9660: Entry bg02.png;1 (A)
iso9660: Entry bg03.png;1 (A)
iso9660: Entry bg04.png;1 (A)
iso9660: Entry bg05.png;1 (A)
iso9660: Entry bg06.png;1 (A)
iso9660: Entry bg07.png;1 (A)
iso9660: Entry boot (4)
iso9660: Entry boot.cat;1 (A)
iso9660: Entry calc.cxx;1 (A)
iso9660: Entry calc.d;1 (8)
iso9660: Entry calc.elf;1 (A)
iso9660: Entry calc.exe;1 (A)
iso9660: End of table
#endif
