#include <string_view>
#include <utility>
#include <algorithm>
#include <optional>
#include "vfs.hxx"
#include "tty.hxx"

static std::optional<Filesystem::Directory> g_RootDir;

void Filesystem::Init()
{
    g_RootDir.emplace();
    
    auto& systemDir = Filesystem::AddDirectory(U"system");
    systemDir.AddFile(U"orthx", Filesystem::File::Type::REPORT);
    systemDir.AddFile(U"servs", Filesystem::File::Type::REPORT);
    systemDir.AddFile(U"msss0", Filesystem::File::Type::REPORT);
    systemDir.AddFile(U"tsss0", Filesystem::File::Type::REPORT);
    systemDir.AddFile(U"ssss0", Filesystem::File::Type::REPORT);
    systemDir.AddFile(U"?", Filesystem::File::Type::REPORT);
    systemDir.GetFile(U"ssss0.");
}

Filesystem::Directory &Filesystem::AddDirectory(const std::u32string_view& name)
{
    return g_RootDir->dirs.emplace_back(std::u32string(name));
}

Filesystem::File& Filesystem::Directory::AddFile(const std::u32string_view& name, Filesystem::File::Type type)
{
    TTY::Print(name.data());
    Filesystem::File file(std::u32string(name), type);
    this->files.push_back(file);
    return this->files.back();
}

Filesystem::Directory *Filesystem::Directory::GetDirectory(const std::u32string_view& name)
{
    for (auto& dir : this->dirs)
        if (dir.name == name)
            return &dir;
    return nullptr;
}

Filesystem::File *Filesystem::Directory::GetFile(const std::u32string_view& path)
{
    std::u32string part;
    std::u32string fullPath(path);

    std::size_t lastPos = 0;
    for (auto pos = fullPath.find_first_of(U'.', 0); pos != std::string::npos; pos = fullPath.find_first_of(U'.', lastPos))
    {
        part = fullPath.substr(lastPos, pos);
        TTY::Print(part.c_str());
        lastPos = pos;
    }

    return nullptr;
}
