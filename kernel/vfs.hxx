#ifndef VFS_HXX
#define VFS_HXX 1

#include <cstddef>

#define MAX_DIRNAME 8
#define MAX_DIR_FILES 32

#define MAX_FILENAME 32

namespace Filesystem
{
    struct File
    {
        char name[MAX_FILENAME] = {};

        File() = default;
        File(File &) = default;
        File(File &&) = default;
        File &operator=(const File &) = default;
        ~File() = default;
    };

    struct Directory
    {
        char name[MAX_DIRNAME] = {};
        File nodes[MAX_DIR_FILES] = {};

        Directory() = default;
        Directory(Directory &) = default;
        Directory(Directory &&) = default;
        Directory &operator=(const Directory &) = default;
        ~Directory() = default;

        Directory *GetDirectory(const char *name);
    };

    Filesystem::Directory &NewDirectory(const char *name);
}

#endif
