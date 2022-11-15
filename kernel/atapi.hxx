#ifndef ATAPI_HXX
#define ATAPI_HXX 1

#include <cstdint>
#include <cstddef>

// The default and seemingly universal sector size for CD-ROMs
#define ATAPI_SECTOR_SIZE 2048

// Valid values for "drive"
#define ATA_DRIVE_MASTER 0xA0
#define ATA_DRIVE_SLAVE 0xB0

namespace ATAPI
{
/// @brief An ATAPI device, note that this is a bus manager so
/// it will be able to manage multiple drives on a single bus
struct Device
{
    enum Bus
    {
        PRIMARY = 0x1F0,
        SECONDARY = 0x170,
    } bus;
    enum Drive
    {
        NONE,
        MASTER = 0xA0,
        SLAVE = 0xB0,
    } drive;
    Device() = default;
    Device(Bus _bus);
    ~Device() = default;

    bool IsPresent();
    bool SendCommand(uint8_t *cmd, size_t size);
    void SelectDrive(Drive _drive);
    int Read(uint32_t lba, uint8_t *buffer, size_t size);
};
}

#endif
