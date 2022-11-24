#include <new>
#include "iso9660.hxx"
#include "atapi.hxx"
#include "locale.hxx"
#include "tty.hxx"

ISO9660::Device::Device(ATAPI::Device &_dev)
    : dev{ _dev }
{

}

std::optional<ISO9660::Device::DirectorySummaryEntry> ISO9660::Device::GetDirEntryLBA(const char *name)
{
    if (!this->dev.Read(0x10, (uint8_t *)&this->pvd, sizeof(this->pvd)))
    {
        TTY::Print("iso9660: Unable to read pvd?\n");
        return std::optional<DirectorySummaryEntry> {};
    }
    TTY::Print("iso9660: Version %u, code %u, LBA L-type %u, opt %u\n", this->pvd.version, this->pvd.code, this->pvd.type_l_path_lba, this->pvd.opt_type_l_path_lba);

    const auto& rootEntry = *std::launder(reinterpret_cast<const ISO9660::DirectoryEntry *>(this->pvd.root_dir_raw));
    TTY::Print("iso9660: Loading root directory LBA 0x%x\n", rootEntry.extent_lba.lsb);
    for(auto currLBA = rootEntry.extent_lba.lsb; ; currLBA++)
    {
        auto readLen = this->dev.Read(currLBA, this->block_buffer, sizeof(this->block_buffer));
        if (!readLen)
            return std::optional<DirectorySummaryEntry> {};

        TTY::Print("iso9660: Read length %x\n", readLen);
        const auto *p = this->block_buffer;
        while ((size_t)(p - this->block_buffer) < sizeof(this->block_buffer))
        {
            const auto& entry = *std::launder(reinterpret_cast<const ISO9660::DirectoryEntry*>(p));
            if (!entry.length)
                break;

            char tmpbuf[entry.MAX_UNIQUE_NAME + 1] = {};
            for (size_t i = 0; i < entry.file_ident_len && i < sizeof(tmpbuf); i++)
                tmpbuf[i] = Locale::Convert<Locale::Charset::ASCII, Locale::Charset::NATIVE>(entry.file_ident[i]);
            tmpbuf[entry.file_ident_len >= sizeof(tmpbuf) ? sizeof(tmpbuf) - 1 : entry.file_ident_len] = '\0';
            TTY::Print("iso9660: Entry %s (%x)\n", tmpbuf, entry.file_ident_len);
            size_t len = sizeof(entry) + entry.file_ident_len;
            if (entry.length < len)
            {
                TTY::Print("iso9660: Entry size is corrupt length=%x, ident_len=%x\n", entry.length, entry.file_ident_len);
                return std::optional<DirectorySummaryEntry> {};
            }
            else
            {
                len = entry.length;
                if (entry.file_ident_len == std::strlen(name) && !std::memcmp(name, tmpbuf, entry.file_ident_len))
                {
                    auto opt = std::optional<DirectorySummaryEntry> {};
                    return opt.emplace(entry.extent_lba.lsb, entry.data_len.lsb);
                }
            }

            p += len;
            if ((size_t)p % 2) // Padding to even boundary
                p++;
        }
    }
    return std::optional<DirectorySummaryEntry> {};
}

bool ISO9660::Device::ReadFile(const char *name, bool (*func)(void *data, size_t len))
{
    auto dirEntry = this->GetDirEntryLBA(name);
    if (dirEntry.has_value())
    {
        TTY::Print("iso9660: File %s len=0x%x, lba=%x\n", name, dirEntry->length, dirEntry->lba);
        auto totalLength = static_cast<signed long>(dirEntry->length);
        auto currLBA = dirEntry->lba;
        while (totalLength > 0)
        {
            const auto len = this->dev.Read(currLBA, this->block_buffer, sizeof(this->block_buffer));
            if (!len)
                return false;
            if (!func(this->block_buffer, len))
                return true;

            TTY::Print("iso9660: File %s len=0x%x, lba=0x%x\n", name, totalLength, currLBA);

            totalLength -= len;
            currLBA++;
        }
        return true;
    }
    TTY::Print("iso9660: File %s not found\n", name);
    return false;
}
