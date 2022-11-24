#include <cstdint>
#include <cstddef>
#include <kernel/vendor.hxx>
#include <kernel/tty.hxx>
#include <kernel/task.hxx>

namespace AdLib
{
class Device
{
    void Write(unsigned char reg, unsigned char val) const;
public:
    Device();
    ~Device() = default;

    bool IsPresent() const;
    //void InitChannel(int channel);
    void Resume();
    void Pause();
    void PlayNote(int channel, unsigned short freq, unsigned char octave);
};
}

#define ADLIB_ADDRESS 0x388
#define ADLIB_STATUS 0x388
#define ADLIB_DATA 0x389

#define ADLIB_REG_ENABLE_WAVEFORM 0x01
#define ADLIB_REG_TIMER_1_DATA 0x02
#define ADLIB_REG_TIMER_2_DATA 0x03
#define ADLIB_REG_TIMER_CONTROL 0x04
#define ADLIB_SPEECH_SYNTHESIS 0x08
#define ADLIB_AMP_MODE 0x20
#define ADLIB_KEY_SCALING 0x40

AdLib::Device::Device()
{
    if (!this->IsPresent())
        return;
    
    TTY::Print("adlib: Reset\n");

    // Reset the soundcard
    for (size_t i = 0; i < 0xFF; i++)
        this->Write(i, 0);
    
    // Magic stuff
    this->Write(0x20, 0x01); // Modulator multiple to 1
    this->Write(0x40, 0x10); // Modulator level to 40db
    this->Write(0x60, 0xF0); // Quick deckay and long modulator attack
    this->Write(0x80, 0x77); // Sustain medium, high, etc
    this->Write(0xA0, 0x98);
    this->Write(0x23, 0x01); // Carriers multiple to 1
    this->Write(0x43, 0x00);
    this->Write(0x63, 0xF0);
    this->Write(0x83, 0x77);
    this->Resume();
    this->Pause();
    TTY::Print("adlib: Initialized\n");
}

bool AdLib::Device::IsPresent() const
{
    // Perform producedure to probe for the card
    this->Write(0x04, 0x60); // Reset both timers
    this->Write(0x04, 0x80); // Enable interrupts
    uint8_t status1 = IO_In8(ADLIB_STATUS);
    this->Write(0x02, 0xFF); // Timer 1
    this->Write(0x04, 0x21); // Start timer 1
    Task::Sleep(80); // Sleep for 80 usec
    uint8_t status2 = IO_In8(ADLIB_STATUS);
    this->Write(0x04, 0x60); // Re-reset both timers
    this->Write(0x04, 0x80); // Re-enable interrupts
    return !(status1 & 0xE0) && (status2 & 0xE0) == 0xC0;
}

void AdLib::Device::Resume()
{
    this->Write(0xB0, 0x31); // Turn voice on, set octave and freq MSB
    TTY::Print("adlib: Resume\n");
}

void AdLib::Device::Pause()
{
    this->Write(0xB0, 0x00); // Turn voice off
    TTY::Print("adlib: Paused\n");
}

void AdLib::Device::Write(unsigned char reg, unsigned char val) const
{
    // Tell adlib what register we want to address
    IO_Out8(ADLIB_ADDRESS, reg);

    // Read status 6 times
    for (size_t i = 0; i < 6; i++)
        IO_In8(ADLIB_ADDRESS);
    
    // Write the value once adlib has acknowledged the register
    // we wanted to write to
    IO_Out8(ADLIB_DATA, val);
    for (size_t i = 0; i < 36; i++)
        IO_In8(ADLIB_ADDRESS);
}

void AdLib::Device::PlayNote(int, unsigned short freq, unsigned char octave)
{
    this->Write(0xA0, freq);
    this->Write(0xB0, (1 << 5) | ((octave & 0x07) << 2) | (freq & 0x03));
    this->Write(0xA8, freq);
    this->Write(0xB8, (1 << 5) | ((octave & 0x07) << 2) | (freq & 0x03));
}
