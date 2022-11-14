module;

#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <utility>
#include <optional>
#include <memory>
#include <new>

export module filesys;

export namespace Filesys
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

    class Directory
    {
        static std::optional<Directory> g_RootDir;
    public:
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

        File& AddFile(const std::u32string_view& name, File::Type type)
        {
            //TTY::Print(name.data());
            File file(std::u32string(name), type);
            this->files.push_back(file);
            return this->files.back();
        }

        Directory *GetDirectory(const std::u32string_view& name)
        {
            for (auto& dir : this->dirs)
                if (dir.name == name)
                    return &dir;
            return nullptr;
        }

        File *GetFile(const std::u32string_view& path)
        {
            std::u32string part;
            std::u32string fullPath(path);

            std::size_t lastPos = 0;
            for (auto pos = fullPath.find_first_of(U'.', 0); pos != std::string::npos; pos = fullPath.find_first_of(U'.', lastPos))
            {
                part = fullPath.substr(lastPos, pos);
                //TTY::Print(part.c_str());
                lastPos = pos;
            }

            return nullptr;
        }

        void RemoveFile(const std::u32string_view& name)
        {
            std::erase_if(this->files, [name](const auto& e) {
                return e.name == name;
            });
        }

        static std::optional<Directory>& GetGlobal()
        {
            return g_RootDir;
        }
    };

    Directory &AddDirectory(const std::u32string_view& name)
    {
        return Directory::GetGlobal()->dirs.emplace_back(std::u32string(name));
    }

    void Init()
    {
        Directory::GetGlobal().emplace();
        auto& systemDir = AddDirectory(U"system");
        systemDir.AddFile(U"orthx", File::Type::REPORT);
        systemDir.AddFile(U"servs", File::Type::REPORT);
        systemDir.AddFile(U"msss0", File::Type::REPORT);
        systemDir.AddFile(U"tsss0", File::Type::REPORT);
        systemDir.AddFile(U"ssss0", File::Type::REPORT);
        systemDir.AddFile(U"?", File::Type::REPORT);
        systemDir.GetFile(U"ssss0.");
    }

    std::optional<Directory> Directory::g_RootDir;
}
