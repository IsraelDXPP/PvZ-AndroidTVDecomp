// ElfShims.h -- the ELF symbol structures the binary walks.
//
// The decompiled code reads its own symbol table: `st_size`, `st_other`,
// `st_shndx`, `st_value` and `st_info` are fields of an ELF32 symbol, reached
// through a pointer the code computes from the module's base address.
//
// On Windows the module is a PE image and there is no such table. This is the
// one place where "reimplement faithfully" is not possible, because the data
// does not exist -- so the structures are declared with the layout the code
// expects, and the lookup finds nothing.
//
// That is the honest outcome rather than a fabrication: the Android build uses
// this to resolve symbol names for crash reports and for the cheat/debug
// console, and on Windows there is nothing to resolve. The code paths that call
// it are guarded by their own success checks in the binary, so an empty table
// makes them take the same "not found" branch they already have.
//
// The layout is ELF32, which is what the target is: 16 bytes per symbol.

#ifndef __ELFSHIMS_H__
#define __ELFSHIMS_H__

#include "SexyTypes.h"

// ---------------------------------------------------------------------------
// Elf32_Sym -- 16 bytes.
//
//   +0   st_name     index into the string table
//   +4   st_value    symbol value (an address for a function)
//   +8   st_size     symbol size
//   +12  st_info     type and binding
//   +13  st_other    visibility
//   +14  st_shndx    section index
// ---------------------------------------------------------------------------
struct Elf32_Sym
{
    std::uint32_t  st_name;
    std::uint32_t  st_value;
    std::uint32_t  st_size;
    _BYTE   st_info;
    _BYTE   st_other;
    _WORD   st_shndx;
};


// The header, for the code that reads the table's location out of it.
struct Elf32_Ehdr
{
    _BYTE   e_ident[16];
    _WORD   e_type;
    _WORD   e_machine;
    std::uint32_t  e_version;
    std::uint32_t  e_entry;
    std::uint32_t  e_phoff;
    std::uint32_t  e_shoff;
    std::uint32_t  e_flags;
    _WORD   e_ehsize;
    _WORD   e_phentsize;
    _WORD   e_phnum;
    _WORD   e_shentsize;
    _WORD   e_shnum;
    _WORD   e_shstrndx;
};

struct Elf32_Shdr
{
    std::uint32_t  sh_name;
    std::uint32_t  sh_type;
    std::uint32_t  sh_flags;
    std::uint32_t  sh_addr;
    std::uint32_t  sh_offset;
    std::uint32_t  sh_size;
    std::uint32_t  sh_link;
    std::uint32_t  sh_info;
    std::uint32_t  sh_addralign;
    std::uint32_t  sh_entsize;
};

struct Elf32_Dyn
{
    std::uint32_t  d_tag;
    std::uint32_t  d_un;
};

// ---------------------------------------------------------------------------
// The linker's symbol list, which the code walks to find a named symbol.
//
// On Windows there is none: the list is empty, and the lookup reports failure.
// The structure is what the code expects to walk, so the loop terminates
// immediately instead of running off the end of a null pointer.
// ---------------------------------------------------------------------------
struct Elf32_SymbolList
{
    const Elf32_Sym*  mSymbols;
    unsigned int      mCount;
    const char*       mStringTable;

    Elf32_SymbolList() : mSymbols(0), mCount(0), mStringTable(0) {}
};

// The empty list the code walks when no table is present.
inline const Elf32_SymbolList& ElfSymbols()
{
    static Elf32_SymbolList sEmpty;
    return sEmpty;
}

#endif // __ELFSHIMS_H__
