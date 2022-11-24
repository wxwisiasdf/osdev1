#ifndef PCI_HXX
#define PCI_HXX 1

#include <cstdint>
#include <vector>
#include <memory>
#include <algorithm>
#include "vendor.hxx"

#define PCI_MAX_SEGMENTS 65535
#define PCI_MAX_BUSES 256
#define PCI_MAX_SLOTS 256
#define PCI_MAX_FUNCS 8
#define PCI_ECAM_SIZE 4096

namespace PCI
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

inline uint32_t Read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
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

inline void Write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
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

inline uint8_t Read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    return (uint8_t)((PCI::Read32(bus, slot, func, offset) >> ((offset % 4) * 8)) & 0xFF);
}

inline uint16_t Read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    return (uint16_t)((PCI::Read32(bus, slot, func, offset) >> ((offset % 2) * 8)) & 0xFFFF);
}

class Device;
class Driver
{
    static std::vector<Driver*> drivers;
public:
    enum Vendor
    {
        LOONGSON = 0x0014,
        PEAK = 0x001C,
        MICROCHIP = 0x0054,
        NOT_FOR_RADIO = 0x0222,
        SHENZHEN = 0x02F3,
        DAQ_SYSTEM = 0x0303,
        CIRRUS = 0x1013,
        IBM = 0x1014,
        UNISYS = 0x1018,
        AMERICAN_MEGATRENDS = 0x101E,
        HEWLETT_PACKARD = 0x103C,
        TEXAS_INSTRUMENTS = 0x104C,
        SONY = 0x104D,
        REALTEK = 0x10EC,
        MIPS = 0x153F,
        TP_LINK = 0x7470,
        INTEL = 0x8086,
        ANY = 0xFFFF,
    } vendor_id = Vendor::ANY;
    uint16_t device_id = 0xFFFF;
    uint8_t class_code = 0xFF;
    uint8_t subclass = 0xFF;

    ~Driver()
    {
        this->RemoveSystem(*this);
    }

    virtual void Init(Device&) {}
    virtual void Deinit(Device&) {}
    virtual void Poweron(Device&) {}
    virtual void Poweroff(Device&) {}

    /// @brief Register driver with the global driver manager
    static void AddSystem(Driver& p)
    {
        drivers.emplace_back(&p);
    }

    /// @brief Remove driver from global system
    static void RemoveSystem(Driver& p)
    {
        drivers.erase(std::remove(std::begin(drivers), std::end(drivers), &p), std::end(drivers));
    }

    static Driver& GetDriver(Vendor vendor_id, uint16_t device_id, uint8_t class_code, uint8_t subclass)
    {
        // Exact matches
        for (const auto& driver : drivers) // Vendor and device
            if (driver->vendor_id == vendor_id && driver->device_id == device_id)
                return *driver;
        for (const auto& driver : drivers) // Class and subclass
            if (driver->class_code == class_code && driver->subclass == subclass)
                return *driver;

        // Non-exact matches (fallbacks)
        for (const auto& driver : drivers)
            if ((driver->vendor_id == Vendor::ANY ? true : driver->vendor_id == vendor_id)
            && (driver->device_id == 0xFFFF ? true : driver->device_id == device_id))
                return *driver;
        for (const auto& driver : drivers)
            if ((driver->class_code == 0xFF ? true : driver->class_code == class_code)
            && (driver->subclass == 0xFF ? true : driver->subclass == subclass))
                return *driver;
        __builtin_unreachable();
    }
};

class Device
{
    static std::vector<Device*> devices;
public:
    uint8_t bus = 0xFF;
    uint8_t slot = 0xFF;
    uint8_t func = 0xFF;

    Driver& GetDriver()
    {
        const auto vendor_id = static_cast<Driver::Vendor>(this->Read16(offsetof(PCI::Header, vendor_id)));
        const auto device_id = this->Read16(offsetof(PCI::Header, device_id));
        const auto class_code = this->Read16(offsetof(PCI::Header, class_code));
        const auto subclass = this->Read16(offsetof(PCI::Header, subclass));
        return Driver::GetDriver(vendor_id, device_id, class_code, subclass);
    }

    void BusMaster(bool value)
    {
        // TODO: bus mastering
    }

    uint32_t Read32(uint8_t offset)
    {
        uint32_t address;
        uint32_t lbus = (uint32_t)this->bus;
        uint32_t lslot = (uint32_t)this->slot;
        uint32_t lfunc = (uint32_t)this->func;
        // Create configuration address as per Figure 1
        address = (uint32_t)((lbus << 16) | (lslot << 11) |
                            (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        IO_Out32(0xCF8, address);  // Write out the address
        auto tmp = IO_In32(0xCFC); // Read in the data
        return tmp;
    }

    void Write32(uint8_t offset, uint32_t data)
    {
        uint32_t address;
        uint32_t lbus = (uint32_t)this->bus;
        uint32_t lslot = (uint32_t)this->slot;
        uint32_t lfunc = (uint32_t)this->func;
        // Create configuration address as per Figure 1
        address = (uint32_t)((lbus << 16) | (lslot << 11) |
                            (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        IO_Out32(0xCF8, address);
        IO_Out32(0xCFC, data);
    }

    uint8_t Read8(uint8_t offset)
    {
        return (uint8_t)((this->Read32(offset) >> ((offset % 4) * 8)) & 0xFF);
    }

    uint16_t Read16(uint8_t offset)
    {
        return (uint16_t)((this->Read32(offset) >> ((offset % 2) * 8)) & 0xFFFF);
    }
};
}

#endif
