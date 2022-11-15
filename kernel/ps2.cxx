module;

import pic;
#include <cstdint>
#include <cstddef>
#include "vendor.hxx"
#include "tty.hxx"
#include "assert.hxx"
import locale;
#include "video.hxx"
#include "task.hxx"

export module ps2;

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

export namespace Keysym
{
consteval char KeyPad(int x)
{
    if(Locale::IsDigit(x))
        return (char)(0x80 + Locale::ToXDigit(x));

    if(x == '+')
        return (char)(0x80 + 10 + 1);
    if(x == '-')
        return (char)(0x80 + 10 + 2);
    if(x == '.')
        return (char)(0x80 + 10 + 3);
    if(x == '*')
        return (char)(0x80 + 10 + 4);
    if(x == '/')
        return (char)(0x80 + 10 + 5);
    return 0;
}

consteval char KeyAltLeft()
{
    return (char)0xEB;
}

consteval char KeyCapsLock()
{
    return (char)0xEC;
}

consteval char KeyScrollLock()
{
    return (char)0xEE;
}

consteval char KeyNumLock()
{
    return (char)0xEF;
}

// Fn is from 0xF0 to 0xFB
consteval char KeyFn(int x)
{
    return (char)(0xF0 + x - 1);
}

consteval char KeyCtrlLeft()
{
    return (char)0xFC;
}

consteval char KeyCtrlRight()
{
    return (char)0xFD;
}

consteval char KeyShiftLeft()
{
    return (char)0xFE;
}

consteval char KeyShiftRight()
{
    return (char)0xFF;
}
};

export namespace PS2
{
struct Keyboard;
static Keyboard *g_ps2_keyboard = nullptr;
struct Mouse;
static Mouse *g_ps2_mouse = nullptr;

struct Controller
{
    enum Command
    {
        READ_BYTE_0 = 0x20,
        WRITE_BYTE_0 = 0x60,
        DISABLE_SECOND = 0xA7,
        ENABLE_SECOND = 0xA8,
        TEST_SECOND_PORT = 0xA9,
        TEST_PS2_CONTROLLER = 0xAA,
        TEST_FIRST_PORT = 0xAB,
        DISABLE_FIRST = 0xAD,
        ENABLE_FIRST = 0xAE,
        READ_OUTPUT = 0xD0,
        WRITE_FIRST = 0xD1,
        WRITE_SECOND = 0xD2,
        WRITE_TO_SECOND = 0xD4,
    };

    enum Response
    {
        ACKNOWLEDGE = 0xFA,
    };

    struct Status
    {
        uint8_t output_buffer_full : 1;
        uint8_t input_buffer_full : 1;
        uint8_t system_flag : 1;
        uint8_t cmd_data : 1;
        uint8_t unknown : 2;
        uint8_t timeout : 1;
        uint8_t parity : 1;

        Status(uint8_t s)
            : output_buffer_full{ static_cast<uint8_t>(s >> 0) },
              input_buffer_full{ static_cast<uint8_t>(s >> 1) },
              system_flag{ static_cast<uint8_t>(s >> 2) },
              cmd_data{ static_cast<uint8_t>(s >> 3) },
              unknown{ static_cast<uint8_t>(s >> 5) },
              timeout{ static_cast<uint8_t>(s >> 6) },
              parity{ static_cast<uint8_t>(s >> 7) }
        {

        }
    };

    bool is_dual = false;

    /// @brief Create and initialize PS2 controller, note that there is only 1 PS2
    /// controller per system so we can use the singleton approach here
    Controller()
    {
        // Disable controller
        IO_Out8(PS2_COMMAND, Controller::Command::DISABLE_FIRST);
        IO_Out8(PS2_COMMAND, Controller::Command::DISABLE_SECOND);
        IO_In8(PS2_DATA); // Flush buffers
        // Read configuration and disable IRQs
        IO_Out8(PS2_COMMAND, Controller::Command::READ_BYTE_0);
        this->WaitInput();
        uint8_t config = IO_In8(PS2_DATA);
        config &= ((uint8_t) ~((1 << 0) | (1 << 1) | (6 << 1)));
        this->is_dual = config & (1 << 5);

        // Write new config value
        IO_Out8(PS2_COMMAND, Controller::Command::WRITE_BYTE_0);
        this->WaitOutput();
        IO_Out8(PS2_DATA, config);

        // Test PS2 controller
        IO_Out8(PS2_COMMAND, Controller::Command::TEST_PS2_CONTROLLER);
        this->WaitInput();
        if (IO_In8(PS2_DATA) != 0x55)
        {
            TTY::Print("ps2: Failed controller test\n");
            return; // Initialization failed
        }
        // Reset configure byte to old value
        IO_Out8(PS2_COMMAND, Controller::Command::WRITE_BYTE_0);
        this->WaitOutput();
        IO_Out8(PS2_DATA, config);

        TTY::Print("ps2: Succeed controller test\n");
        if (this->is_dual)
        {
            // Check that it is indeed a dual channel controller
            IO_Out8(PS2_COMMAND, Controller::Command::ENABLE_SECOND);
            IO_Out8(PS2_COMMAND, Controller::Command::READ_BYTE_0);
            this->WaitInput();
            config = IO_In8(PS2_DATA);
            this->is_dual = !(config & (1 << 5)); // Bit 5 should be clear

            if (this->is_dual)
            {
                // Disable again iff dual channel
                IO_Out8(PS2_COMMAND, Controller::Command::DISABLE_SECOND);
                TTY::Print("ps2: Dual channel\n");
            }
        }

        // TODO: Use the other port if one fails & it's a dual channel
        TTY::Print("ps2: Perform tests\n");
        this->WaitOutput();
        IO_Out8(PS2_COMMAND, Controller::Command::TEST_FIRST_PORT);
        if (IO_In8(PS2_DATA) != 0) // First port failed
            TTY::Print("ps2: First port failed\n");

        if (this->is_dual)
        {
            this->WaitOutput();
            IO_Out8(PS2_COMMAND, Controller::Command::TEST_SECOND_PORT);
            if (IO_In8(PS2_DATA) != 0) // Second port failed
                TTY::Print("ps2: Second port failed\n");
        }

        // Enable ports
        this->WaitOutput();
        IO_Out8(PS2_COMMAND, Controller::Command::ENABLE_FIRST);
        if (this->is_dual)
        {
            this->WaitOutput();
            IO_Out8(PS2_COMMAND, Controller::Command::ENABLE_SECOND);
        }

        TTY::Print("ps2: Initialized\n");
    }

    Controller(Controller &) = delete;
    Controller(Controller &&) = delete;
    ~Controller() = default;

    void WaitInput() const
    {
        IO_TimeoutWait(500, [this]()
        {
            return !this->GetStatus().input_buffer_full;
        });
    }

    void WaitOutput() const
    {
        IO_TimeoutWait(500, [this]()
        {
            return !this->GetStatus().output_buffer_full;
        });
    }

    Controller::Status GetStatus() const
    {
        return Controller::Status(IO_In8(PS2_STATUS));
    }

    void WriteFirst(unsigned char data) const
    {
        this->WaitInput();
        IO_Out8(PS2_DATA, data);
    }

    void WriteSecond(unsigned char data) const
    {
        IO_Out8(PS2_COMMAND, Controller::Command::WRITE_TO_SECOND);
        this->WaitInput();
        IO_Out8(PS2_DATA, data);
    }

    void WriteFirstOrSecond(unsigned char data, bool second) const
    {
        if (second)
            this->WriteSecond(data);
        else
            this->WriteFirst(data);
    }

    void WriteController(unsigned short index, uint8_t (*fn)(uint8_t config)) const
    {
        this->PollRead(); // Flush outb

        IO_Out8(PS2_COMMAND, Controller::Command::READ_BYTE_0 + index);
        this->WaitInput();
        const auto byte = fn(IO_In8(PS2_DATA));
        IO_Out8(PS2_COMMAND, Controller::Command::WRITE_BYTE_0 + index);
        this->WaitOutput();
        IO_Out8(PS2_DATA, byte);
    }

    uint8_t PollRead() const
    {
        this->WaitOutput();
        return IO_In8(PS2_DATA);
    }
};

const static char ps2_set1[] =
{
    0,
    0x1B,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'Q',
    'W',
    'E',
    'R',
    'T',
    'Y',
    'U',
    'I',
    'O',
    'P',
    '[',
    ']',
    '\n',
    Keysym::KeyCtrlLeft(),
    'A',
    'S',
    'D',
    'F',
    'G',
    'H',
    'J',
    'K',
    'L',
    ';',
    '\'',
    '`',
    Keysym::KeyShiftLeft(),
    '\\',
    'Z',
    'X',
    'C',
    'V',
    'B',
    'N',
    'M',
    ',',
    '.',
    '/',
    Keysym::KeyShiftRight(),
    Keysym::KeyPad('*'),
    Keysym::KeyAltLeft(),
    ' ',
    Keysym::KeyCapsLock(),
    Keysym::KeyFn(1),
    Keysym::KeyFn(2),
    Keysym::KeyFn(3),
    Keysym::KeyFn(4),
    Keysym::KeyFn(5),
    Keysym::KeyFn(6),
    Keysym::KeyFn(7),
    Keysym::KeyFn(8),
    Keysym::KeyFn(9),
    Keysym::KeyFn(10),
    Keysym::KeyNumLock(),
    Keysym::KeyScrollLock(),
    Keysym::KeyPad('7'),
    Keysym::KeyPad('8'),
    Keysym::KeyPad('9'),
    Keysym::KeyPad('-'),
    Keysym::KeyPad('4'),
    Keysym::KeyPad('5'),
    Keysym::KeyPad('6'),
    Keysym::KeyPad('+'),
    Keysym::KeyPad('1'),
    Keysym::KeyPad('2'),
    Keysym::KeyPad('3'),
    Keysym::KeyPad('0'),
    Keysym::KeyPad('.'),
    0,
    0,
    0,
    Keysym::KeyFn(11),
    Keysym::KeyFn(12),
};

struct Keyboard
{
    Controller &controller;
    bool capslock = false;
    bool shift = false;
    /// @brief Whatever this is first or second port
    bool first = false;

    static constexpr auto max_key_bufsize = 64;
    /// @brief Ringbuffer for storing keys
    char buf[max_key_bufsize] = {};
    /// @brief Current index in ringbuffer
    size_t n_buf = 0;

    Keyboard(Controller &_controller)
        : controller{_controller}
    {
        TTY::Print("ps2: Initializing keyboard\n");
        g_ps2_keyboard = this;

        this->controller.WriteFirst(0xFF);
        this->controller.PollRead();
        this->controller.WriteFirst(0xF5); // Reset default parameters
        this->controller.PollRead();
        this->controller.WriteFirst(0xF3);         // Set typematic delay
        if (this->controller.PollRead() == Controller::Response::ACKNOWLEDGE)
            TTY::Print("ps2_keyboard: Set typematic delay\n");
        this->controller.WriteFirst(0b0'11'11111); // 11 = 1000ms delay
        if (this->controller.PollRead() == Controller::Response::ACKNOWLEDGE)
            TTY::Print("ps2_keyboard: Set typematic rate\n");
        this->controller.WriteFirst(0xF4); // Enable scanning
        if (this->controller.PollRead() == Controller::Response::ACKNOWLEDGE)
            TTY::Print("ps2_keyboard: Enable scanning\n");

        TTY::Print("ps2: Enabling keyboard IRQs\n");
        this->controller.WriteController(0, [](uint8_t config) -> uint8_t
        {
            return config | (1 << 0); // Enable IRQs
        });
        PIC::Get().SetIRQMask(1, false); // And tell PIC to enable line
    }

    Keyboard(Keyboard &) = delete;
    Keyboard(Keyboard &&) = delete;
    Keyboard &operator=(const Keyboard &) = delete;

    ~Keyboard()
    {
        // TODO: Unregister
        if (g_ps2_keyboard == this)
            g_ps2_keyboard = nullptr;
    }

    int GetPollKey()
    {
        auto ch = static_cast<char>(this->controller.PollRead());
        bool released = (ch & 0x80) != 0;
        ch = ps2_set1[(ch & (~0x80)) % sizeof(ps2_set1)]; // Translate
        switch (ch)
        {
        case Keysym::KeyCapsLock():
            this->capslock = true;
            break;
        case Keysym::KeyShiftLeft():
        case Keysym::KeyShiftRight():
            this->shift = released;
            break;
        default:
            if (released) // Do not prompt on release events
                return 0;
            if (this->shift || this->capslock)
                return ch;
            return Locale::ToLower(ch);
        }
        return 0;
    }

    int GetKey()
    {
        int ch = this->buf[this->n_buf];
        this->buf[this->n_buf++] = '\0';
        if (this->n_buf >= sizeof(this->buf))
            this->n_buf = 0;
        return ch;
    }

    static Keyboard &Get()
    {
        assert(g_ps2_keyboard != nullptr);
        return *g_ps2_keyboard;
    }
};

#define PS2_MOUSE_LEFT_BUTTON 0
#define PS2_MOUSE_RIGHT_BUTTON 1
#define PS2_MOUSE_MIDDLE_BUTTON 2
struct Mouse
{
    enum Command
    {
        SET_SCALING_1_1 = 0xE6,
        SET_SCALING_2_1 = 0xE7,
        SET_RESOLUTION = 0xE8,
        STATUS_REQUEST = 0xE9,
        SET_STREAM_MODE = 0xEA,
        REQUEST_SINGLE_PACKET = 0xEB,
        SET_WRAP_MODE = 0xEE,
        RESET_WRAP_MODE = 0xEC,
        SET_REMOTE_MODE = 0xF0,
        GET_MOUSE_ID = 0xF2,
        SET_SAMPLE_RATE = 0xF3,
        ENABLE_PACKET_STREAM = 0xF4,
        DISABLE_PACKET_STREAM = 0xF5,
        SET_DEFAULTS = 0xF6,
        RESEND = 0xFE,
        RESET = 0xFF,
    };
    Controller &controller;
    /// @brief Whatever this is first or second port
    bool first = false;
    /// @brief State of each button, pressed/released
    bool buttons[5] = {false, false, false, false};
    unsigned x = 0, y = 0;

    Mouse(Controller &_controller)
        : controller{_controller}
    {
        TTY::Print("ps2: Initializing mouse\n");
        g_ps2_mouse = this;

        //this->controller.WriteSecond(Mouse::RESET);
        //if (this->controller.PollRead() == Controller::Response::ACKNOWLEDGE)
        //    TTY::Print("ps2_mouse: Reset\n");
        this->controller.WriteSecond(Mouse::SET_DEFAULTS);
        if (this->controller.PollRead() == Controller::Response::ACKNOWLEDGE)
            TTY::Print("ps2_mouse: Set defaults\n");
        this->controller.WriteSecond(Mouse::ENABLE_PACKET_STREAM);
        if (this->controller.PollRead() == Controller::Response::ACKNOWLEDGE)
            TTY::Print("ps2_mouse: Enable packet stream\n");

        // Enable IRQs
        TTY::Print("ps2: Enabling mouse IRQs\n");
        this->controller.WriteController(0, [](uint8_t config) -> uint8_t
        {
            return config | (1 << 1); // Enable IRQs
        });
        PIC::Get().SetIRQMask(12, false); // And tell PIC to enable line
    }

    Mouse(Mouse &) = delete;
    Mouse(Mouse &&) = delete;
    Mouse &operator=(const Mouse &) = delete;

    ~Mouse()
    {
        // TODO: Unregister
        if (g_ps2_mouse == this)
            g_ps2_mouse = nullptr;
    }

    void PollRead()
    {
        struct MousePacket
        {
            union
            {
                struct
                {
                    uint8_t bl : 1;
                    uint8_t br : 1;
                    uint8_t bm : 1;
                    uint8_t ao : 1;
                    uint8_t xs : 1;
                    uint8_t ys : 1;
                    uint8_t xo : 1;
                    uint8_t yo : 1;
                } data;
                uint8_t status;
            };
            uint8_t xm;
            uint8_t ym;
        } PACKED;
        // Follows an entire mouse packet...
        MousePacket packet;
        packet.status = this->controller.PollRead();
        if (!packet.data.ao)
            return;
        packet.xm = this->controller.PollRead();
        packet.ym = this->controller.PollRead();
        // Drop packet if X/Y overflow
        if (packet.data.xo || packet.data.yo)
            return;

        const auto rel_x = static_cast<signed int>(packet.xm) - (packet.data.xs * 0x100);
        const auto rel_y = static_cast<signed int>(packet.ym) - (packet.data.ys * 0x100);

        signed long sx = (signed long)this->x + rel_x;
        signed long sy = (signed long)this->y + -rel_y;
        sx = sx < 0 ? 0 : sx; // Limit to 0
        sy = sy < 0 ? 0 : sy;
        this->x = sx;
        this->y = sy;

        this->buttons[PS2_MOUSE_LEFT_BUTTON] = packet.data.bl;
        this->buttons[PS2_MOUSE_RIGHT_BUTTON] = packet.data.br;
        this->buttons[PS2_MOUSE_MIDDLE_BUTTON] = packet.data.bm;
    }

    int GetX() const
    {
        return this->x;
    }

    int GetY() const
    {
        return this->y;
    }

    static Mouse &Get()
    {
        assert(g_ps2_mouse != nullptr);
        return *g_ps2_mouse;
    }
};
}

/// @brief Keyboard IRQ handler
extern "C" void IntE9h_Handler()
{
    Task::DisableSwitch();
#ifdef DEBUG
    TTY::Print("ps2: Handling IRQ E9 for keyboard\n");
#endif
    auto &kb = PS2::Keyboard::Get();
    kb.buf[kb.n_buf++] = kb.GetPollKey();
    if (kb.n_buf >= sizeof(kb.buf))
        kb.n_buf = 0;
    kb.buf[kb.n_buf] = '\0';
    Task::Schedule();
    PIC::Get().EOI(1);
    Task::EnableSwitch();
}

/// @brief Mouse IRQ handler
extern "C" void IntF4h_Handler()
{
    Task::DisableSwitch();
#ifdef DEBUG
    TTY::Print("ps2: Handling IRQ F4 for mouse\n");
#endif
    auto &mouse = PS2::Mouse::Get();
    mouse.PollRead();
    if (mouse.x > g_KFrameBuffer.width - 8)
        mouse.x = g_KFrameBuffer.width - 8;
    if (mouse.y > g_KFrameBuffer.height - 8)
        mouse.y = g_KFrameBuffer.height - 8;
    g_KFrameBuffer.MoveMouse(mouse.GetX(), mouse.GetY());
    Task::Schedule();
    PIC::Get().EOI(12);
    Task::EnableSwitch();
}

