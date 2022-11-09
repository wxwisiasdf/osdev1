#ifndef VFS_HXX
#define VFS_HXX 1

#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <memory>
#include <new>

namespace Filesystem
{
    struct File
    {
        std::u32string name;
        enum Type
        {
            DISK_FILE,
            REPORT,
        } type;

        File() = default;
        File(const std::u32string& _name, Type _type)
            : name{ _name },
            type{ _type }
        {

        }
        File(const File &) = default;
        File(File &&) = default;
        File &operator=(const File &) = default;
        File &operator=(File &&) = default;
        ~File() = default;
    };

    struct Directory
    {
        std::u32string name;
        std::vector<File> files;
        std::vector<Directory> dirs;

        Directory() = default;
        Directory(const std::u32string& _name)
            : name{ _name }
        {

        }
        Directory(const Directory &) = default;
        Directory(Directory &&) = default;
        Directory &operator=(const Directory &) = default;
        Directory &operator=(Directory &&) = default;
        ~Directory() = default;

        File& AddFile(const std::u32string_view& name, Filesystem::File::Type type);
        Directory *GetDirectory(const std::u32string_view& name);

        void RemoveFile(const std::u32string_view& name)
        {
            std::erase_if(this->files, [name](const auto& e) {
                return e.name == name;
            });
        }

        File *GetFile(const std::u32string_view& path);
    };

    void Init();
    Filesystem::Directory &AddDirectory(const std::u32string_view& name);
}

#endif
