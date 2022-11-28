/// sysex.cxx
/// System executor management

#include <cstdint>
#include <cstddef>
#include <optional>
#include <kernel/appkit.hxx>
#include <kernel/ui.hxx>
#include <kernel/pic.hxx>
#include <kernel/pci.hxx>
#include <kernel/vendor.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>
#include <kernel/assert.hxx>
#include <kernel/drm.hxx>
#include <kernel/alloc.hxx>
#include <kernel/iso9660.hxx>
#include <kernel/atapi.hxx>
#include <kernel/ps2.hxx>

extern std::optional<UI::Desktop> g_Desktop;

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

extern std::optional<PS2::Controller> ps2Controller;
extern std::optional<PS2::Keyboard> ps2Keyboard;
extern std::optional<PS2::Mouse> ps2Mouse;
extern std::optional<ATAPI::Device> atapiDevices[2];
extern std::optional<ISO9660::Device> isoCdrom;
int UDOS_32Main(char32_t[])
{
    // Sometimes kernel_main gets executed twice
    Task::DisableSwitch();
    PIC::Get().Remap(0xE8, 0xF0);
    Task::EnableSwitch();
    asm("sti"); // Always enable interrupts on the dummy task
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
    // 0x1000000;
    g_Desktop.emplace();
    g_Desktop->x = 0;
    g_Desktop->y = 0;
    g_Desktop->width = g_KFrameBuffer.width;
    g_Desktop->height = g_KFrameBuffer.height;
    UI::Manager::Get().SetRoot(*g_Desktop);

    HimemAlloc::Print(HimemAlloc::Manager::GetDefault());

    auto& exeIcon = g_Desktop->AddChild<UI::DesktopIcon>();
    exeIcon.x = 64;
    exeIcon.y = 64;
    exeIcon.width = 64;
    exeIcon.height = 64;
    exeIcon.SetText("Hello world");

    auto& taskbar = g_Desktop->AddChild<UI::Taskbar>();
    taskbar.x = 0;
    taskbar.y = g_KFrameBuffer.height - 20;
    taskbar.width = g_KFrameBuffer.width;
    taskbar.height = 20;

    static char serialKey[15] = { 'U', 'D', 'O', 'S', 'B', 'E', 'S', 'T', 'O', 'S', 'E', 'V', 'E', 'R', '\0' };
    static uint32_t serialKeyParray[32];
    static uint32_t serialKeySbox[4][256];
    DRM::Blowfish::GenKey(serialKeyParray, serialKeySbox, serialKey, std::strlen(serialKey));

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

        auto& serialKeyBtn = startMenu->AddChild<UI::Button>();
        serialKeyBtn.flex = startMenu->flex;
        serialKeyBtn.x = serialKeyBtn.y = 0;
        serialKeyBtn.width = startMenu->width - 12;
        serialKeyBtn.height = 12;
        serialKeyBtn.SetText("Enter serial key");
        serialKeyBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
            auto& serialKeyWin = g_Desktop->AddChild<UI::Window>();;
            serialKeyWin.SetText("Input your unique serial key");
            serialKeyWin.x = serialKeyWin.y = 0;
            serialKeyWin.width = 200;
            serialKeyWin.height = 64;
            serialKeyWin.Decorate();

            static std::optional<UI::Textbox> serialKeyTextbox;
            serialKeyTextbox.emplace();
            serialKeyTextbox->x = serialKeyTextbox->y = 0;
            serialKeyTextbox->width = serialKeyWin.width - 12;
            serialKeyTextbox->height = 12;
            serialKeyTextbox->SetText("");
            serialKeyTextbox->SetTooltipText("Your serial key\nHint: It's 'SERIAL0'");
            serialKeyTextbox->OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {

            });
            serialKeyWin.AddChildDirect(serialKeyTextbox.value());
            
            auto& serialKeyBtn = serialKeyWin.AddChild<UI::Button>();
            serialKeyBtn.x = 0;
            serialKeyBtn.y = 16;
            serialKeyBtn.width = serialKeyWin.width - 12;
            serialKeyBtn.height = 12;
            serialKeyBtn.SetText("Check");
            serialKeyBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
                for(size_t i = 0; i < sizeof(serialKey); i++)
                    serialKey[i] = serialKeyTextbox->textBuffer[i];

                if(!DRM::Blowfish::IsValidSerialKey(*serialKey))
                {
                    UI::Manager::Get().ErrorMsg(*"DRM", *"Serial key is invalid, proceeding to crash the OS");
                }
            });
        });

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

            static Task::TSS* runTask = nullptr;
            static bool performRunTask = false;
            auto& runBtn = runWin.AddChild<UI::Button>();
            runBtn.x = 0;
            runBtn.y = 16;
            runBtn.width = runWin.width - 12;
            runBtn.height = 12;
            runBtn.SetText("Run");
            runBtn.OnClick = ([](UI::Widget &, unsigned, unsigned, bool, bool) -> void {
                if(runTask != nullptr)
                    performRunTask = true;
                
                runTask = &Task::Add([]() -> void  {
                doRunTask:
                    static char tmpbuf[100];
                    for(size_t i = 0; i < 100; i++)
                        tmpbuf[i] = filepathTextbox->textBuffer[i];

                    static auto* imageBase = (void *)0x1000000;
                    static size_t offset = 0;
                    auto r = isoCdrom->ReadFile("chess.exe;1", [](void *data, size_t len) -> bool {
                        TTY::Print("Reading 0x%x bytes at %p\n", len, (uint8_t *)imageBase + offset);
                        //std::memcpy((uint8_t *)imageBase + offset, data, len);
                        DRM::Blowfish::Decrypt((uint8_t *)imageBase + offset, serialKeyParray, serialKeySbox, data, len);
                        offset += len;
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
                        TTY::Print("Executing at %p!\n", imageBase);
                        const auto* info = reinterpret_cast<AppKit::ProgramInfo*>(imageBase);
                        int r = info->entryPoint(nullptr);
                        //Task::Add(, nullptr, false);
                    }

                    while(!performRunTask)
                        ;
                    goto doRunTask;
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
            Task::Add([]()
            {
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
                infoTextbox.OnUpdate = [](UI::Widget& w)
                {
                    auto& o = static_cast<UI::Textbox&>(w);
                    auto fmtPrint = [](const char *fmt, ...)
                    {
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
    });

    TTY::Print("System ready! Welcome to UDOS!\n");
    HimemAlloc::Print(HimemAlloc::Manager::GetDefault());
    // Filesys::Init();

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
    return 0;
}

__attribute__((section(".data.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
