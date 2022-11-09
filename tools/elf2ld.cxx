#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <elf.h>

static size_t ShdrOffset(const Elf32_Ehdr& ehdr, size_t i)
{
    return ehdr.e_shoff + i * ehdr.e_shentsize;
}

static std::string GetSymbolName(FILE* fp, const Elf32_Ehdr& ehdr, size_t idx, size_t sidx)
{
    if(sidx == SHN_UNDEF)
        throw std::runtime_error("Undefined string section");

    std::fseek(fp, ShdrOffset(ehdr, sidx), SEEK_SET);
    Elf32_Shdr shdr;
    std::fread(&shdr, sizeof(shdr), 1, fp);
    std::fseek(fp, shdr.sh_offset + idx, SEEK_SET);

    static char tmpbuf[1024];
    std::fread(tmpbuf, sizeof(tmpbuf[0]), sizeof(tmpbuf), fp);
    return std::string(tmpbuf);
}

static std::string GetSectionName(FILE* fp, const Elf32_Ehdr& ehdr, size_t idx)
{
    return GetSymbolName(fp, ehdr, idx, ehdr.e_shstrndx);
}

static void DoSymbolTable(FILE* fp, const Elf32_Ehdr& ehdr, const Elf32_Shdr& shdr)
{
    for (size_t i = 0; i < shdr.sh_size / shdr.sh_entsize; i++)
    {
        std::fseek(fp, shdr.sh_offset + i * shdr.sh_entsize, SEEK_SET);
        Elf32_Sym sym;
        std::fread(&sym, sizeof(sym), 1, fp);

        std::string name = GetSymbolName(fp, ehdr, sym.st_name, shdr.sh_link);
        if (!name.empty())
        {
            if (sym.st_shndx == SHN_ABS)
            {
                std::printf("%s = 0x%08x;\n", name.c_str(), sym.st_value);
            }
            else
            {
                std::printf("%s = 0x%08x;\n", name.c_str(), sym.st_value);
            }
        }
    }
}

int main(int argc, char **argv)
{
    try
    {
        for (int i = 1; i < argc; i++)
        {
            Elf32_Ehdr ehdr;
            std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(argv[i], "rb"), fclose);
            std::fread(&ehdr, sizeof(ehdr), 1, fp.get());

            if(ehdr.e_ident[EI_MAG0] != ELFMAG0
            || ehdr.e_ident[EI_MAG1] != ELFMAG1
            || ehdr.e_ident[EI_MAG2] != ELFMAG2
            || ehdr.e_ident[EI_MAG3] != ELFMAG3)
                throw std::runtime_error("Not an elf file");
            
            // Go to section table
            for (size_t j = 0; j < ehdr.e_shnum; j++)
            {
                std::fseek(fp.get(), ShdrOffset(ehdr, j), SEEK_SET);
                Elf32_Shdr shdr;
                std::fread(&shdr, sizeof(shdr), 1, fp.get());

                std::string name = GetSectionName(fp.get(), ehdr, shdr.sh_name);
                std::cout << "/* section " << name << " " << shdr.sh_name << " */" << std::endl;

                if (shdr.sh_type == SHT_SYMTAB)
                {
                    DoSymbolTable(fp.get(), ehdr, shdr);
                }
            }
        }
    }
    catch(std::exception& e)
    {
        std::cout << "exception" << e.what() << std::endl;
    }
    return 0;
}
