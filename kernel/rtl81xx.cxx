module;

import pci;
#include <cstdint>
#include <cstddef>
#include "tty.hxx"
#include "vendor.hxx"

export module rtl81xx;

// 8K+16 = Minimum buffer for RTL
#define RX_BUFFER_SIZE 8192 + 16
// 64K is enough for everything
#define TX_BUFFER_SIZE 0xFFFF
// TODO: Driver broken, please fix, recv is broken. send is seemingly
// broken but i cannot validate
#define RCR_WRAP 1 << 7
#define MAX_TX_CHUNK_SIZE 1792 // Max TX bytes that can be sent at once

export namespace RTL81XX
{
    struct Device : public PCI::Device
    {
        uint8_t mac[6];
        uint16_t io_base;
        uint32_t dma_base;
        char rx_buffer[RX_BUFFER_SIZE];
        char tx_buffer[TX_BUFFER_SIZE];
        char rr_counter; // Round robin counter

        Device(const PCI::Device &dev)
        {
            this->bus = dev.bus;
            this->slot = dev.slot;
            this->func = dev.func;
            this->io_base = PCI::Read16(this->bus, this->slot, this->func, offsetof(PCI::Header, bar[0]));
            this->dma_base = PCI::Read32(this->bus, this->slot, this->func, offsetof(PCI::Header, bar[1]));

            IO_Out8(this->io_base + 0x52, 0x00); // Turn on the device
            IO_Out8(this->io_base + 0x37, 0x10); // Do a software reset
            while (IO_In8(this->io_base + 0x37) & 0x10)
                ;

            this->rr_counter = 0;
            // Set the physical address of the buffer
            IO_Out32(this->io_base + 0x30, (uintptr_t)this->rx_buffer); // Low
            IO_Out32(this->io_base + 0x30 + 4, 0);                      // High
            IO_Out32(this->io_base + 0x44, 0x0F | RCR_WRAP);            // Use ring buffer
            IO_Out16(this->io_base + 0x3C, 0x0005);                     // Enable TOK and ROK bits for enabling IRQs
            IO_Out32(this->io_base + 0x44, 0x8F);                       // Enable receive buffers
            IO_Out8(this->io_base + 0x37, 0x0C);                        // Enable Receive and Transmit bits

            // Read the MAC address
            this->mac[0] = IO_In8(this->io_base + 0x00);
            this->mac[1] = IO_In8(this->io_base + 0x01);
            this->mac[2] = IO_In8(this->io_base + 0x02);
            this->mac[3] = IO_In8(this->io_base + 0x03);
            this->mac[4] = IO_In8(this->io_base + 0x04);
            this->mac[5] = IO_In8(this->io_base + 0x05);

            // TODO: Configure IRQ
        }

        void Receive()
        {
            TTY::Print("rtl81xx: hello networking world\n");
            // ACK that we sucessfully received the packet
            IO_Out8(this->io_base + 0x3E, 0x01);
        }

        void Send(unsigned char *buf, size_t n)
        {
            // Send packets - if they are bigger than 1792 bytes we send
            // them in chunks
            for (size_t i = 0; i < (n / MAX_TX_CHUNK_SIZE) + 1; i++)
            {
                // Fill buffer
                for (size_t j = 0; j < sizeof(this->tx_buffer); j++)
                    this->tx_buffer[i] = buf[i + (i * MAX_TX_CHUNK_SIZE)];
                IO_Out32(this->io_base + 0x20 + (this->rr_counter * 4), (uintptr_t)this->tx_buffer & 0xFFFFFFFF);
                IO_Out32(this->io_base + 0x10 + (this->rr_counter * 4), n % MAX_TX_CHUNK_SIZE);
                this->rr_counter++;
                if (this->rr_counter > 3)
                    this->rr_counter = 0;
                // TODO: This hangs, why? and how i can fix this?
                // while (!(IO_In32(this->io_base + 0x10 + (this->rr_counter * 4)) & 0x800))
                //     ;
            }
        }
    };
}
