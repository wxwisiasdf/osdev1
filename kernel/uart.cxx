module;

#include <cstdint>
#include <optional>
#include "vendor.hxx"

export module uart;

export namespace UART
{
    struct Device
    {
        unsigned short io_base;
        Device() = default;

        Device(unsigned short base_io)
            : io_base{base_io}
        {
            IO_Out8(this->io_base + 1, 0x00); // Disable all interrupts
            IO_Out8(this->io_base + 3, 0x80); // Enable DLAB (set baud rate divisor)
            IO_Out8(this->io_base + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
            IO_Out8(this->io_base + 1, 0x00); //                  (hi byte)
            IO_Out8(this->io_base + 3, 0x03); // 8 bits, no parity, one stop bit
            IO_Out8(this->io_base + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
            IO_Out8(this->io_base + 4, 0x0B); // IRQs enabled, RTS/DSR set
            IO_Out8(this->io_base + 4, 0x1E); // Set in loopback mode, test the serial chip
            IO_Out8(this->io_base + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

            // Check if serial is faulty (i.e: not same byte as sent)
            if (IO_In8(this->io_base + 0) != 0xAE)
                return;

            // If serial is not faulty set it in normal operation mode
            // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
            IO_Out8(this->io_base + 4, 0x0F);
        }

        Device(Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(const Device&&) = delete;
        ~Device() = default;

        bool IsReceiveFull() const
        {
            return IO_In8(this->io_base + 5) & 1;
        }

        char GetChar() const
        {
            while (!this->IsReceiveFull())
                ;
            return IO_In8(this->io_base);
        }

        bool IsTransmitFull() const
        {
            return IO_In8(this->io_base + 5) & 0x20;
        }

        void Send(char ch) const
        {
            while (!this->IsTransmitFull())
                ;
            IO_Out8(this->io_base, ch);
            IO_Out8(0xE9, ch);
        }
    };
    std::optional<UART::Device> com[2];
}
