#include "pic.hxx"
#include "atapi.hxx"
#include "assert.hxx"
#include "vendor.hxx"
#include "tty.hxx"
#include "task.hxx"

// The necessary I/O ports, indexed by "bus"
#define ATA_DATA(x) (x)
#define ATA_FEATURES(x) (x + 1)
#define ATA_SECTOR_COUNT(x) (x + 2)
#define ATA_ADDRESS1(x) (x + 3)
#define ATA_ADDRESS2(x) (x + 4)
#define ATA_ADDRESS3(x) (x + 5)
#define ATA_DRIVE_SELECT(x) (x + 6)
#define ATA_COMMAND(x) (x + 7)
#define ATA_DCR(x) (x + 0x206) // device control register

ATAPI::Device::Device(ATAPI::Device::Bus _bus)
    : bus{_bus},
    drive{ ATAPI::Device::Drive::NONE }
{
    if (this->bus == Bus::PRIMARY)
        PIC::Get().SetIRQMask(14, false);
    else
        PIC::Get().SetIRQMask(15, false);

    IO_Out8(this->bus + 0x206, 1 << 2); // Perform a software reset
    IO_In8(this->bus + 0x206); // Wait 100ns
    IO_Out8(this->bus + 0x206, 0); // Clear SRST back
    this->SelectDrive(ATAPI::Device::Drive::MASTER);

    if (!this->IsPresent())
        TTY::Print("atapi: No drives present on bus %x\n", this->bus);
}

/// @brief Checks if the device is present at all on the given bus
/// if it isn't then it will send all-high volt signals read as 0xFF
/// basically, if it's 0xFF, then there are no drives here
/// @return If there is any drive present on this bus
bool ATAPI::Device::IsPresent()
{
    this->SelectDrive(ATAPI::Device::Drive::MASTER);
    // Set LBA to 0
    for (size_t i = 0; i < 4; i++)
        IO_Out8(this->bus + 2 + i, 0x00);
    IO_Out8(this->bus + 7, 0xEC); // IDENTIFY command
    if(IO_In8(this->bus + 7) == 0x00) // Check status
    {
        TTY::Print("atapi: Empty status, definitely no ATA here\n");
        return false;
    }

    // Immediately setting an error on ATA command IDENTIFY means this is ATAPI
    uint8_t status = IO_In8(this->bus + 7);
    if (status & 0x1)
    {
        TTY::Print("atapi: Immediately set status to %x after IDENTIFY\n", status);
        return true;
    }

    if (!IO_TimeoutWait(1000, [this, &status]() -> bool
{
    status = IO_In8(this->bus + 7);
        return !(status & 0x80); }))
    {
        TTY::Print("atapi: Busy timeout\n");
        return false;
    }

    if (!IO_TimeoutWait(1000, [this, &status]() -> bool
{
    status = IO_In8(this->bus + 7);
        return status & 0x8; }))
    {
        TTY::Print("atapi: Data request bit timeout\n");
        return false;
    }

    if (status & 0x1)
    {
        TTY::Print("atapi: Unexpected error after polling\n");
        return false;
    }

    // TODO: Store identify data
    for (size_t i = 0; i < 256; i++)
        IO_In16(this->bus);
    return true;
}

static bool dataReady[2] = {false, false};

/// @brief Sends a command to the ATAPI device
/// @param cmd Command buffer chain to send
/// @param size Length of chain
/// @return Whetever the command succeeded on send or not
bool ATAPI::Device::SendCommand(uint8_t *cmd, size_t size)
{
    TTY::Print("WARNING: ENABLING THE FUCKING INTERRUPTS\n");
    asm("sti");

    if (!this->IsPresent())
        return false;

    IO_Out8(this->bus + 1, 0x0); // PIO mode
    IO_Out8(this->bus + 4, ATAPI_SECTOR_SIZE & 0xFF);
    IO_Out8(this->bus + 5, ATAPI_SECTOR_SIZE >> 8);
    IO_Out8(this->bus + 7, 0xA0); // ATA PACKET command

    uint8_t status;
    TTY::Print("atapi: Waiting for clear busy\n");
    if (!IO_TimeoutWait(1000, [this, &status]() -> bool
{
    status = IO_In8(this->bus + 7);
        return !(status & 0x80); }))
    return false;

    // Wait for DRQ to be set (indicates drive is ready for PIO data)
    if (!IO_TimeoutWait(1000, [this, &status]() -> bool
{
    status = IO_In8(this->bus + 7);
        return status & 0x8; }))
    return false;

    // DRQ or ERROR set
    if (status & 0x1)
        return false;

    TTY::Print("atapi: Sending command %x (%u words)\n", cmd[0], size / 2);
    // Send ATAPI/SCSI command
    for (size_t i = 0; i < size / sizeof(uint16_t); i++)
        IO_Out16(this->bus, ((uint16_t *)cmd)[i]);

    // Wait for IRQ that says the data is ready
    auto& isReady = dataReady[this->bus == ATAPI::Device::Bus::PRIMARY ? 0 : 1];
    TTY::Print("atapi: Wait for IRQ\n");
    if (!IO_TimeoutWait(30 * 1000, [this, &isReady]() -> bool
{ return isReady; }))
    {
        TTY::Print("atapi: IRQ never arrived\n");
        return false;
    }
    isReady = false;
    TTY::Print("atapi: Command sent succesfully\n");

    asm("cli");
    return true;
}

void ATAPI::Device::SelectDrive(ATAPI::Device::Drive _drive)
{
    if (this->drive == _drive) // Avoid huge delay
        return;

    this->drive = _drive;
    TTY::Print("atapi: Switching bus %x to drive %x\n", this->bus, this->drive);
    // Select drive (only the slave-bit is set)
    IO_Out8(this->bus + 6, this->drive);
    // ATA specifies a 400ns delay after drive switching -- often
    // implemented as 4 Alternative Status queries
    for (size_t i = 0; i < 4; i++)
        IO_In8(this->bus + 0x206);
}

/// @brief Use the ATAPI protocol to read a single sector from the given
/// bus/drive into the buffer using logical block address lba.
/// @param lba Logical block address to read
/// @param buffer Buffer to place read data into
/// @param size Size of read
/// @return Size of the read
int ATAPI::Device::Read(uint32_t lba, uint8_t *buffer, size_t size)
{
    // 0xA8 is READ SECTORS command byte
    static uint8_t readCmd[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    readCmd[9] = 1;                    // 1 sector
    readCmd[2] = (lba >> 0x18) & 0xFF; // most sig. byte of LBA
    readCmd[3] = (lba >> 0x10) & 0xFF;
    readCmd[4] = (lba >> 0x08) & 0xFF;
    readCmd[5] = (lba >> 0x00) & 0xFF; // least sig. byte of LBA
    if (!this->SendCommand(readCmd, sizeof(readCmd)))
        return 0;

    // Read actual size
    size_t readSize = (IO_In8(ATA_ADDRESS3(this->bus)) << 8) | IO_In8(ATA_ADDRESS2(this->bus));
    TTY::Print("atapi: Size=%u,SectorSize=%u\n", readSize, ATAPI_SECTOR_SIZE);
    if (!readSize)
        return 0;
    // This example code only supports the case where the data transfer
    // of one sector is done in one step
    assert(readSize >= ATAPI_SECTOR_SIZE);
    size = size > readSize ? readSize : size;
    // Read data
    for (size_t i = 0; i < size / sizeof(uint16_t); i++)
        ((uint16_t *)buffer)[i] = IO_In16(ATA_DATA(this->bus));
    // Discard extra data
    for (size_t i = size / sizeof(uint16_t); i < readSize / sizeof(uint16_t); i++)
        IO_In16(this->bus);

    // The controller will send another IRQ even though we've read all
    // the data we want. Wait for it -- so it doesn't interfere with
    // subsequent operations
    // TODO: ^^^

    // Wait for BSY and DRQ to clear, indicating Command Finished
    //uint8_t status;
    //if (IO_TimeoutWait(10000, [this, &status]() -> bool
    //                   { return !((status = IO_In8(ATA_COMMAND(this->bus))) & 0x88); }))
    //    return 0;
    TTY::Print("atapi: Read %u bytes (%u discarded)\n", size, readSize - size);
    return readSize;
}

extern "C" void IntF6h_Handler()
{
    Task::DisableSwitch();
    TTY::Print("atapi: Handling F6\n");
    dataReady[0] = true;
    PIC::Get().EOI(14);
    Task::EnableSwitch();
}

extern "C" void IntF7h_Handler()
{
    Task::DisableSwitch();
    TTY::Print("atapi: Handling F7\n");
    dataReady[1] = true;
    PIC::Get().EOI(15);
    Task::EnableSwitch();
}
