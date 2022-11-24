#include <cstdint>
#include <cstddef>
#include <kernel/appkit.hxx>
#include <kernel/pci.hxx>
#include <kernel/vendor.hxx>
#include <kernel/task.hxx>
#include <kernel/tty.hxx>
#include <kernel/assert.hxx>

struct IntelHDA_Driver : PCI::Driver
{
    struct MemRegs {
        uint16_t globalCap;
        uint8_t minorVer;
        uint8_t majorVer;
        uint16_t outPayloadCap;
        uint16_t inPayloadCap;
        uint32_t globalCtl;
        uint16_t wakeEnable;
        uint16_t stateChangeStatus;
        uint16_t globalStatus;
        uint8_t reserved1[5];
        uint16_t outStreamPayloadCap;
        uint16_t inStreamPayloadCap;
        uint32_t reserved2;
        uint32_t intCtl;
        uint32_t intStatus;
        uint8_t reserved3[8];
        uint32_t wallClock;
        uint8_t reserved4[4];
        uint32_t streamSync;
        uint8_t reserved5[4];
        uint32_t corbLoAddr;
        uint32_t corbHiAddr;
        uint16_t corbWriteP;
        uint16_t corbReadP;
        uint8_t corbCtl;
        uint8_t corbStatus;
        uint8_t corbSize;
        uint8_t reserved6;
        uint32_t rirbLoAddr;
        uint32_t rirbHiAddr;
        uint16_t rirbWriteP;
        uint16_t rirbIntCount;
        uint8_t rirbCtl;
        uint8_t rirbStatus;
        uint8_t rirbSize;
        uint8_t reserved7;
        uint32_t immCmd;
        uint32_t immResp;
        uint16_t immStatus;
        uint8_t reserved8[6];
        uint32_t dmaLoAddr;
        uint32_t dmaHiAddr;
        uint8_t reserved9[8];
        struct PerStreamData
        {
            uint16_t ctlLo;
            uint8_t ctlHi;
            uint8_t status;
            uint32_t linkPos;
            uint32_t cyclicBufferLength;
            uint16_t lastValidIndex;
            uint16_t fifoWatermark;
            uint16_t fifoSize;
            uint16_t descrFormat;
            uint8_t reserved1[4];
            uint32_t descrListLoAddr;
            uint32_t descrListHiAddr;

            void SetHz(int hz)
            {
                bool lowerSampleRate = false;
                bool dividing = true;
                auto divisor = 48000.f / static_cast<float>(hz);
                auto multiple = 0.f;

                decltype(descrFormat) toWrite = 0;
                if(divisor < 0.f)
                {
                    dividing = false;
                    multiple = static_cast<float>(hz) / 44800.f;
                    if(static_cast<float>(static_cast<int>(multiple)) != multiple)
                    {
                        lowerSampleRate = true;
                        multiple = static_cast<float>(hz) / 44100.f;
                    }
                }
                else if(static_cast<float>(static_cast<int>(divisor)) != divisor)
                {
                    lowerSampleRate = true;
                    divisor = 44100.f / static_cast<float>(hz);
                }

                if(dividing)
                {
                    toWrite = (static_cast<uint16_t>(lowerSampleRate) << 14)
                        | ((static_cast<uint16_t>(divisor) - 1) << 8);
                }
                else
                {
                    toWrite = (static_cast<uint16_t>(lowerSampleRate) << 14)
                        | ((static_cast<uint16_t>(multiple) - 1) << 11);
                }
                this->descrFormat = (this->descrFormat & 0b111'1111) | toWrite;
            }
        };
        struct PerStreamData streams[];
    };
    volatile MemRegs* mmap = nullptr;

    void SetDMAAddr(void *addr)
    {
        const auto uaddr = reinterpret_cast<uintptr_t>(addr);
        assert((uaddr & 0x7F) == 0);
        this->mmap->dmaLoAddr = uaddr;
        this->mmap->dmaHiAddr = 0;
    }

    virtual void Init(PCI::Device& dev)
    {
        int a = (offsetof(MemRegs, immStatus));
        TTY::Print("hda: Init\n");
        this->mmap = reinterpret_cast<decltype(this->mmap)>(dev.Read32(offsetof(PCI::Header, bar[0])));
        assert(this->mmap != reinterpret_cast<decltype(this->mmap)>(0xFFFFFFFF));
        TTY::Print("hda: Version %u.%u\n", this->mmap->majorVer, this->mmap->minorVer);

        const auto numOutStreams = (this->mmap->globalCap >> 12) & 0b1111;
        const auto numInStreams = (this->mmap->globalCap >> 8) & 0b1111;
        const auto numBiStreams = (this->mmap->globalCap >> 3) & 0b1111;
        TTY::Print("hda: NStreams:Out=%u,In=%u,Bi=%u\n", numOutStreams, numInStreams, numBiStreams);
        TTY::Print("hda: PayloadCap:Out=%u,In=%u.Streams:Out=%u,In=%u\n", this->mmap->outPayloadCap, this->mmap->inPayloadCap, this->mmap->outStreamPayloadCap, this->mmap->inStreamPayloadCap);
    }

    virtual void Deinit(PCI::Device&)
    {
        TTY::Print("hda: Deinit\n");
    }

    virtual void Poweron(PCI::Device&)
    {
        TTY::Print("hda: Poweron\n");
    }

    virtual void Poweroff(PCI::Device&)
    {
        TTY::Print("hda: Poweroff\n");
    }
};

int UDOS_32Main(char32_t[])
{
    static IntelHDA_Driver driver{};
    driver.vendor_id = PCI::Driver::Vendor::INTEL;
    driver.class_code = 0x04;
    driver.subclass = 0x03;

    PCI::Driver::AddSystem(driver);

    TTY::Print("iHDA driver online! :3\n");
    while(1)
        Task::Switch();
    TTY::Print("iHDA driver says goodbye! :3\n");
    return 0;
}

__attribute__((section(".data.startup"))) AppKit::ProgramInfo pgInfo = {
    .entryPoint = &UDOS_32Main
};
