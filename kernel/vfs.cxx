#include "vfs.hxx"

#define MAX_DIRS 64 // Max directories in the entire systems

static Filesystem::Directory g_Dirs[MAX_DIRS] = {};

Filesystem::Directory &Filesystem::NewDirectory(const char *name)
{
    for (size_t i = 0; i < MAX_DIRS; i++)
    {
        auto &dir = g_Dirs[i];
        if (dir.name[0] == '\0')
        {
            for (size_t j = 0; j < MAX_DIRNAME; j++)
            {
                dir.name[j] = name[j];
                if (name[j] == '\0')
                    break;
            }
            return dir;
        }
    }
    __builtin_unreachable();
}

Filesystem::Directory *Filesystem::Directory::GetDirectory(const char *name)
{
    for (size_t i = 0; i < MAX_DIRS; i++)
    {
        auto &dir = g_Dirs[i];
        bool found = true;
        for (size_t j = 0; j < MAX_DIRNAME; j++)
        {
            if (dir.name[j] != name[j])
            {
                found = false;
                break;
            }
        }

        if (found)
            return &g_Dirs[i];
    }
    return nullptr;
}
