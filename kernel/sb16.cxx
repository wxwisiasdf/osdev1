module;

import pic;
import dma;
import pci;
#include <cstdint>
#include "audio.hxx"
#include "vendor.hxx"
#include "assert.hxx"
#include "task.hxx"

export module sb16;


// Soundblaster IRQ table
static const unsigned char irqToNumber[4][2] =
{
    { 0x01, 2 }, // Jumper value - IRQ
    { 0x02, 5 },
    { 0x04, 7 },
    { 0x08, 10 }
};

export struct SoundBlaster16 : public Audio::Driver
{
    enum ChannelMode
    {
        SINGLE = 0x48,
        AUTO = 0x58,
    };

    enum TransferMode
    {
        PLAYSOUND = 0x00,
        RECORDSOUND = 0x01,
    };
private:
    void InitChannel(int channel, ChannelMode chanMode);
    void WriteDSP(uint8_t value)
    {
        IO_TimeoutWait(100, [this]() -> bool
        {
            return IO_In8(0x20E + this->base) & 0x80;
        });
        IO_Out8(0x20C + this->base, value);
    }

    void PlayBuffer(int channel, int hz, void *data, size_t len)
    {
        assert(channel < 4);
        DMA::Bus::StartTransfer(data, len, channel, DMA::ChanMode::AUTO);

        // Set SB16 sample rates
        hz = this->sampleRate * 2;
        this->WriteDSP(0x40);
        size_t nChannels = 1;
        size_t timeConstant = 256 - (1000000 / (nChannels * static_cast<size_t>(hz)));
        this->WriteDSP(timeConstant);

        // Send mode to the SB16
        this->WriteDSP(this->mode); // Transfer mode
        this->WriteDSP(0x00); // Sound type
        // Lenght of data - 1
        this->WriteDSP(len - 1);
        this->WriteDSP((len >> 8) - 1);
    }
public:
    enum PCMMode
    {
        PCM_8 = 0xC0, // 8-bits PCM
        PCM_16 = 0xB0, // 16-bits PCM
    } mode = PCM_8; // Current mode of the soundcard
    ChannelMode channels[4];
    unsigned dsp_version = 0;
    int base; // ISA base of the soundblaster
    unsigned int sampleRate;
    static SoundBlaster16 *singleton;

    SoundBlaster16() = default;
    SoundBlaster16(int _base, int, TransferMode inout, PCMMode _mode)
        : mode{ _mode },
          base{ _base },
          sampleRate{ 44100 }
    {
        this->singleton = this;

        // Send first DSP reset
        IO_Out8(0x206 + this->base, 1);
        // Wait 3ms
        for (size_t i = 0; i < 30000; i++)
            ;

        // Do last DSP reset
        IO_Out8(0x206 + this->base, 0);
        IO_TimeoutWait(3000, [this]() -> bool
        {
            return IO_In8(0x20E + this->base) & 0x80;
        });
        // Read final 0xAA code
        if (IO_In8(0x20A + this->base) != 0xAA)
            return;

        // Obtain Soundblaster DSP version
        this->WriteDSP(0xE1);
        this->dsp_version = IO_In8(0x20A + this->base) << 8;
        this->dsp_version |= IO_In8(0x20A + this->base);
        TTY::Print("sb16: DSP version %x\n", this->dsp_version);

        // Send IRQ to send data to (choose IRQ5)
        IO_Out8(0x204 + this->base, 0x80);
        IO_Out8(0x205 + this->base, irqToNumber[1][0]);
        PIC::Get().SetIRQMask(5, false);

        switch (inout)
        {
        case TransferMode::PLAYSOUND:
            this->WriteDSP(0xD1); // Start the speaker
            break;
        case TransferMode::RECORDSOUND:
            break;
        default:
            break;
        }
    }
    SoundBlaster16(SoundBlaster16&) = delete;
    SoundBlaster16(SoundBlaster16&&) = delete;
    SoundBlaster16& operator=(SoundBlaster16&) = delete;
    ~SoundBlaster16() = default;

    void Start()
    {
        this->FlushBuffer();
    }

    void Finish()
    {

    }

    void Pause()
    {
        switch (this->mode)
        {
        case PCM_8:
            this->WriteDSP(0xD0);
            break;
        case PCM_16:
            this->WriteDSP(0xD5);
            break;
        }
    }

    void Resume()
    {
        switch (this->mode)
        {
        case PCM_8:
            this->WriteDSP(0xD4);
            break;
        case PCM_16:
            this->WriteDSP(0xD6);
            break;
        }
    }

    void FlushBuffer()
    {
        void* buffer = (void *)0x500;
        this->PlayBuffer(1, this->sampleRate, buffer, this->sampleRate);
        if (this->FillBuffer)
            this->FillBuffer(buffer, this->sampleRate);
    }

    void (*FillBuffer)(void *buffer, size_t len) = nullptr;
};
SoundBlaster16* SoundBlaster16::singleton = nullptr;

/// @brief SoundBlaster IRQ handler
extern "C" void IntEDh_Handler()
{
    Task::DisableSwitch();

    TTY::Print("sb16: Handling IRQ ED for soundcard\n");
    if (SoundBlaster16::singleton)
    {
        auto* sb16 = SoundBlaster16::singleton;
        sb16->FlushBuffer();
        IO_In8(0x20E + sb16->base); // Acknowledge to the DSP
    }

    Task::Schedule();
    PIC::Get().EOI(5);
    Task::EnableSwitch();
}
