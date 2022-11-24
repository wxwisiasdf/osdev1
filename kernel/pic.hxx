#ifndef PIC_HXX
#define PIC_HXX 1

#include <cstdint>
#include "vendor.hxx"
#include "tty.hxx"
#include "assert.hxx"

#define PIC1 0x20 // IO base address for master PIC
#define PIC2 0xA0 // IO base address for slave PIC
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

class PIC
{
    static PIC pic;
public:
    enum Command1
    {
        ICW4 = 0x01, // ICW4 on/off
        SINGLE = 0x02, // Single or cascade
        INTERVAL = 0x04, // Call address
        LEVEL = 0x08, // Level edge mode
        INIT = 0x10, // Initialize
    };

    enum Command4
    {
        I8086 = 0x01, // 8086/88 mode
        AUTO = 0x02, // Auto/Normal EOI
        BUF_SLVAE = 0x08, // Buffered mode/slave
        BUF_MASTER = 0x0C, // Buffered mode/master
        SFNM = 0x10, // Special (or not) fully nested
    };

    int master_irq_base;
    int slave_irq_base;

    PIC() = default;
    PIC(PIC&) = delete;
    PIC(PIC&&) = delete;
    PIC& operator=(const PIC&) = delete;
    ~PIC() = default;

    void Remap(int master_off, int slave_off)
    {
        assert((master_off & 0x07) == 0);
        assert((slave_off & 0x07) == 0);

        static uint8_t mask[2];
        mask[0] = IO_In8(PIC1_DATA); // Save masks
        mask[1] = IO_In8(PIC2_DATA);

        IO_Out8(PIC1_COMMAND, PIC::Command1::INIT | PIC::Command1::ICW4); // starts the initialization sequence (in cascade mode)
        IO_Wait();
        IO_Out8(PIC2_COMMAND, PIC::Command1::INIT | PIC::Command1::ICW4);
        IO_Wait();
        IO_Out8(PIC1_DATA, master_off); // ICW2: Master PIC vector offset
        IO_Wait();
        IO_Out8(PIC2_DATA, slave_off); // ICW2: Slave PIC vector offset
        IO_Wait();
        IO_Out8(PIC1_DATA, 4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
        IO_Wait();
        IO_Out8(PIC2_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
        IO_Wait();

        IO_Out8(PIC1_DATA, PIC::Command4::I8086);
        IO_Wait();
        IO_Out8(PIC2_DATA, PIC::Command4::I8086);
        IO_Wait();

        IO_Out8(PIC1_DATA, mask[0]);
        IO_Out8(PIC2_DATA, mask[1]);

        this->master_irq_base = master_off;
        this->slave_irq_base = slave_off;
    }

    void SetIRQMask(unsigned irq, bool masked)
    {
        assert(irq < 0x10); // 16-irqs only
        uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
        uint8_t value = IO_In8(port);
        irq = irq >= 8 ? irq - 8 : irq;
        if (masked)
            value |= (1 << irq);
        else
            value &= ~(1 << irq);
        IO_Out8(port, value);
    }

    void EOI(unsigned irq) const
    {
        assert(irq < 0x10); // 16-irqs only
        IO_Out8(PIC1_COMMAND, 0x20);
        if (irq >= 8)
            IO_Out8(PIC2_COMMAND, 0x20);
    }
    
    static PIC& Get()
    {
        return pic;
    }
};

#endif
