module;

#include <cstdint>
#include "vendor.hxx"

export module pci;

#define PCI_MAX_SEGMENTS 65535
#define PCI_MAX_BUSES 256
#define PCI_MAX_SLOTS 256
#define PCI_MAX_FUNCS 8
#define PCI_ECAM_SIZE 4096

export namespace PCI
{
    struct Capability
    {
        enum Type
        {
            NONE,
            POWER_MANAGEMENT,
            AGP, // Acelerated graphics
            VPD, // Vital product data
            SLOT_IDENT,
            MSI,                // Message signaled interupts
            COMPACTPCI_HOTSWAP, // Hotswap-capable
        };
    };

    struct Header
    {
        uint16_t vendor_id;
        uint16_t device_id;
        struct Command
        {
            uint16_t io_space : 1;
            uint16_t mem_space : 1;
            uint16_t bus_master : 1;
            uint16_t special_cycles : 1;
            uint16_t mem_wi_enable : 1;
            uint16_t vga_palette_snoop : 1;
            uint16_t parity_err : 1;
            uint16_t reserved : 1;
            uint16_t serr_enable : 1;
            uint16_t fast_btb_enable : 1;
            uint16_t interrupt_disable : 1;
            uint16_t reserved2 : 5;
        } command;
        uint16_t status;
        uint8_t revision_id;
        uint8_t prog_if;
        uint8_t subclass;
        uint8_t class_code;
        uint8_t cache_line_size;
        uint8_t latency_timer;
        uint8_t header_type;
        uint8_t bist;
        uint32_t bar[6];
        uint32_t cardbus_cis;
        uint16_t subsystem_vendor_id;
        uint16_t subsystem_id;
        uint32_t expansion_rom_address;
        uint8_t capabilities;
        uint8_t reserved[3];
        uint32_t reserved2;
        uint8_t interrupt_line;
        uint8_t interrupt_pin;
        uint8_t min_grant;
        uint8_t max_latency;
    };

    uint32_t Read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
    {
        uint32_t address;
        uint32_t lbus = (uint32_t)bus;
        uint32_t lslot = (uint32_t)slot;
        uint32_t lfunc = (uint32_t)func;
        // Create configuration address as per Figure 1
        address = (uint32_t)((lbus << 16) | (lslot << 11) |
                            (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        IO_Out32(0xCF8, address);  // Write out the address
        auto tmp = IO_In32(0xCFC); // Read in the data
        return tmp;
    }

    void Write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                    uint32_t data)
    {
        uint32_t address;
        uint32_t lbus = (uint32_t)bus;
        uint32_t lslot = (uint32_t)slot;
        uint32_t lfunc = (uint32_t)func;
        // Create configuration address as per Figure 1
        address = (uint32_t)((lbus << 16) | (lslot << 11) |
                            (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        IO_Out32(0xCF8, address);
        IO_Out32(0xCFC, data);
    }

    uint8_t Read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
    {
        return (uint8_t)((PCI::Read32(bus, slot, func, offset) >> ((offset % 4) * 8)) & 0xFF);
    }

    uint16_t Read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
    {
        return (uint16_t)((PCI::Read32(bus, slot, func, offset) >> ((offset % 2) * 8)) & 0xFFFF);
    }

    struct Device
    {
        uint8_t bus;
        uint8_t slot;
        uint8_t func;
    };
}
