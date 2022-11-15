#ifndef ISO9660_HXX
#define ISO9660_HXX 1

#include <cstdint>
#include <cstring>
#include <optional>
#include "vendor.hxx"
#include "atapi.hxx"

namespace ISO9660
{
template <typename T>
struct BiendianValue
{
    T lsb;
    T msb;
} PACKED;

struct VolumeDateTime
{
    uint8_t year[4];
    uint8_t month[2];
    uint8_t day[2];
    uint8_t hour[2];
    uint8_t minute[2];
    uint8_t second[2];
    uint8_t centisecond[2];
    uint8_t timezone_offset; // Offset from GMT in 15 minute intervals
} PACKED;
static_assert(sizeof(ISO9660::VolumeDateTime) == 17);

struct DateTime
{
    uint8_t year;            // Years since 1900
    uint8_t month;           // 1 to 12
    uint8_t day;             // 1 to 31
    uint8_t hour;            // 0 to 23
    uint8_t minute;          // 0 to 59
    uint8_t second;          // 0 to 59
    uint8_t timezone_offset; // Offset from GMT in 15 minute intervals
} PACKED;
static_assert(sizeof(ISO9660::DateTime) == 7);

struct PathEntry
{
    uint8_t length;
    uint8_t extended_attrib;
    uint32_t extent_lba;
    uint16_t dirnum_parent_dir;
    uint8_t dir_ident[0];
} PACKED;

struct DirectoryEntry
{
    static constexpr auto MAX_UNIQUE_NAME = 24;

    uint8_t length;
    uint8_t extended_attrib;
    ISO9660::BiendianValue<uint32_t> extent_lba;
    ISO9660::BiendianValue<uint32_t> data_len;
    ISO9660::DateTime record;
    uint8_t flags;
    uint8_t unit_size;
    uint8_t interleave_gap_size;
    ISO9660::BiendianValue<uint16_t> volume_seq_num;
    uint8_t file_ident_len;
    uint8_t file_ident[0];
} PACKED;
static_assert(sizeof(ISO9660::DirectoryEntry) == 33);

struct VolumeDescriptor
{
    uint8_t code;
    uint8_t identifier[5];
    uint8_t version;
} PACKED;

struct PrimaryVolumeDescriptor : public ISO9660::VolumeDescriptor
{
    uint8_t unused1;
    uint8_t system_ident[32];
    uint8_t volume_ident[32];
    uint8_t unused2[8];
    ISO9660::BiendianValue<uint32_t> space_size;
    uint8_t unused3[32];
    ISO9660::BiendianValue<uint16_t> set_size;
    ISO9660::BiendianValue<uint16_t> seq_number;
    ISO9660::BiendianValue<uint16_t> block_size;
    ISO9660::BiendianValue<uint32_t> table_size;
    uint32_t type_l_path_lba;
    uint32_t opt_type_l_path_lba;
    uint32_t type_m_path_lba;
    uint32_t opt_type_m_path_lba;
    union
    {
        //ISO9660::DirectoryEntry root_dir;
        uint8_t root_dir_raw[34];
    };
    uint8_t volume_set_ident[128];
    uint8_t publisher_ident[128];
    uint8_t data_preparer_ident[128];
    uint8_t app_ident[128];
    uint8_t copyright_file_ident[37];
    uint8_t abstract_file_ident[37];
    uint8_t bibliographic_file_ident[37];
    ISO9660::VolumeDateTime creation;
    ISO9660::VolumeDateTime modification;
    ISO9660::VolumeDateTime expiration;
    ISO9660::VolumeDateTime effective;
    uint8_t file_structure_ver;
    uint8_t unused4;
    uint8_t application_used[512];
    uint8_t unused5[653];
} PACKED;
static_assert(sizeof(ISO9660::PrimaryVolumeDescriptor) == 2048);

class Device
{
    ATAPI::Device &dev;
    ISO9660::PrimaryVolumeDescriptor pvd = {};
    uint8_t block_buffer[2048] = {};
    struct DirectorySummaryEntry
    {
        uint32_t lba;
        uint32_t length;
    };
    std::optional<DirectorySummaryEntry> GetDirEntryLBA(const char *name);

public:
    Device(ATAPI::Device &_dev);
    Device(Device &) = delete;
    Device(Device &&) = delete;
    Device &operator=(const Device &&) = delete;
    ~Device() = default;
    bool ReadFile(const char *name, bool (*func)(void *data, size_t len));
};
}

#endif
