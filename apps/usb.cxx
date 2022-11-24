#include <cstdint>
#include <cstddef>
#include <kernel/appkit.hxx>
#include <kernel/pci.hxx>
#include <kernel/vendor.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>

// TODO: Is this really the best way to code?
// TODO: Note that programming interface (IF) of the PCI devices contain 0x20 for USB 2.0 (ehci)
// maybe make this fucking usb system more dynamic?
// TODO: Finish the driver
// TODO: Read even more the spec, don't leave shit midway >:(

namespace EHCI
{
constexpr auto USBLEGSUP = 0;
constexpr auto USBLEGCTLSTS = 4;
constexpr auto USBLEGSUP_HC_OS = 1 << 24;
constexpr auto USBLEGSUP_HC_BIOS = 1 << 16;
constexpr auto USBCMD_HC_RESET = 1 << 1;
constexpr auto USBCMD_RS = 1 << 0;
constexpr auto USBSTS_HC_HALT = 1 << 12;
constexpr auto HCSPARAMS_PPC = 0b1 << 4;
constexpr auto HCSPARAMS_N_PORTS = 0b1111;

struct QueueHead
{
    uint32_t queue_ptr;
};

struct Device : public PCI::Device
{
    void *base = nullptr;
    volatile struct HCCRegs
    {
        uint8_t caplen;
        uint8_t reserved;
        uint16_t hci_version;
        uint32_t hci_struct;
        uint32_t hci_cap;
        uint64_t hcsp_postroute;
    } *hccregs = nullptr;
    volatile struct OpRegs
    {
        uint32_t usbcmd;
        uint32_t usbsts;
        uint32_t intr;
        uint32_t frindex;
        uint32_t ctrlseg;
        uint32_t framelst;
        uint32_t asyncaddr;
        uint32_t reserved[9];
        uint32_t config;
        uint32_t ports[];
    } *opregs = nullptr;
    uint8_t n_companion;
    volatile void *plist_base = nullptr;
    volatile void *async_base = nullptr;
    size_t eecp;
    char fp[1024] ALIGN(4096);

    Device(const PCI::Device &dev)
        : PCI::Device(dev),
          base{ reinterpret_cast<decltype(base)>(this->Read32(offsetof(PCI::Header, bar[0]))) },
          hccregs{ reinterpret_cast<decltype(hccregs)>(this->base) },
          opregs{ reinterpret_cast<decltype(opregs)>(reinterpret_cast<uint8_t *>(this->base) + this->hccregs->caplen) }
    {
        if(this->base == reinterpret_cast<decltype(base)>(0xFFFFFFFF))
        {
            TTY::Print("ehci: Unsetup base at invalid offset\n");
            return;
        }

        TTY::Print("ehci: Base at %p\n", this->base);

        // We will reset the controller. When we do this the host controller
        // will reset everything it has internally. Note we cannot restart a
        // actively running host controller; so we will first check the USB
        // status for HALT bit.
        if (this->opregs->usbsts & USBSTS_HC_HALT)
            this->opregs->usbcmd ^= USBCMD_HC_RESET;
        Task::Switch();

        // Reclaim EHCI device from firmware/BIOS
        this->eecp = (this->hccregs->hci_cap & 0xFF00) >> 8;
        TTY::Print("ehci: eecp at %p\n", this->eecp);
        if (this->eecp >= 0x40)
        {
            uintptr_t lsup = this->Read32(this->eecp + USBLEGSUP);
            if (!(lsup & USBLEGSUP_HC_OS))
            {
                lsup |= USBLEGSUP_HC_OS;
                this->Write32(this->eecp + USBLEGSUP, lsup);
                lsup = this->Read32(this->eecp + USBLEGSUP);
                while (lsup & USBLEGSUP_HC_BIOS)
                {
                    lsup = this->Read32(this->eecp + USBLEGSUP);
                    TTY::Print("ehci: waiting to reclaim...\n");
                }
            }
            TTY::Print("ehci: reclaimed host controller from bios\n");
        }

        TTY::Print("ehci: hccregs at %p\n", this->hccregs);
        TTY::Print("ehci: opregs at %p\n", this->opregs);

        // Port control registers is at opregs+0x44
        size_t nports = this->hccregs->hci_struct & 7;
        TTY::Print("ehci: %u ports found\n", nports);

        // Number of companion host controllers
        this->n_companion = (this->hccregs->hci_struct >> 12) & 7;
        TTY::Print("ehci: %u companion controllers\n", this->n_companion);

        uint16_t opreg_port = (uintptr_t)this->opregs + 0x44;
        for (size_t i = 0; i < nports; i++)
        {
            // Reset port, first write the bit, then read the zero bit back
            this->opregs->ports[i] |= 1 << 8;
            this->opregs->ports[i] ^= 1 << 8;
            Task::Switch();

            // Wait until the reset sequence ends
            while (this->opregs->ports[i] & (1 << 8))
                Task::Switch();

            TTY::Print("ehci: %u port has been reset\n", i);

            /* Is a device attached? */
            uint32_t preg = this->opregs->ports[i] & 1;
            TTY::Print("ehci: %u port has %s present on it\n", i, (preg) ? "a device" : "nothing");

            opreg_port += 4;
        }

        // Configure the IRQ
        // pci_configure_irq(current_device, &ehci_receive);
        // Tell ehci root controller to run the schedule
        TTY::Print("ehci: running new schedule\n");
        this->opregs->usbcmd |= USBCMD_RS;
        Task::Switch();
    }
};
}

int UDOS_32Main(char32_t[])
{
    TTY::Print("USB driver online! ^-^\n");
    while(1)
        Task::Switch();
    TTY::Print("USB driver says goodbye! ^-^ :3\n");
    return 0;
}

__attribute__((section(".data.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
