#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <elf.h>
#include <sys/mman.h>
#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

struct elf_symbol {
  word_t addr;
  uint64_t size;
  char name[128];
};

struct elf_symbol elfsts[256];

int sts_len = 0;

static void* mmapfile(const char* filename) {
  FILE *pyfile = fopen(filename, "r");
  if (pyfile == NULL) {
    perror("fopen()");
    return 0;
  }
  if (fseek(pyfile, 0, SEEK_END)) {
    perror("fseek()");
    fclose(pyfile);
    return 0;
  }
  long pyfile_size = ftell(pyfile);

  void *pybytes = mmap(NULL, (size_t)pyfile_size, PROT_READ, MAP_PRIVATE,
                       fileno(pyfile), 0);
  if (pybytes == NULL) {
    fclose(pyfile);
    perror("mmap()");
    return 0;
  }
  fclose(pyfile);

  return pybytes;
}

static int parseelf(void* pybytes, struct elf_symbol* sts) {
  const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
  Elf32_Ehdr elf_hdr;
  memmove(&elf_hdr, pybytes, sizeof(elf_hdr));
  if (memcmp(elf_hdr.e_ident, expected_magic, sizeof(expected_magic)) != 0) {
    perror("Target is not an ELF executable\n");
    return 1;
  }
  if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS32) {
    perror("Sorry, only ELF-32 is supported.\n");
    return 1;
  }

  char *cbytes = (char *)pybytes;

  size_t dynstr_off = 0;
  size_t strtab_off = 0;
  size_t symtab_sz = 0;
  size_t symtab_off = 0;
  int findstrtab = 0;

  for (uint16_t i = 0; i < elf_hdr.e_shnum; i++) {
    size_t offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;
    Elf32_Shdr shdr;
    memmove(&shdr, pybytes + offset, sizeof(shdr));
    switch (shdr.sh_type) {
      case SHT_SYMTAB:
        symtab_off = shdr.sh_offset;
        symtab_sz = shdr.sh_size;
        break;
      case SHT_STRTAB:
        if (!dynstr_off) {
          dynstr_off = shdr.sh_offset;
        } else if (!findstrtab) {
          strtab_off = shdr.sh_offset;
          findstrtab = 1;
        }
        break;
      default:
        break;
    }
  }
  assert(dynstr_off);
  assert(strtab_off);

  int sts_idx = 0;
  for (size_t j = 0; j * sizeof(Elf32_Sym) < symtab_sz; j++) {
    Elf32_Sym sym;
    size_t absoffset = symtab_off + j * sizeof(Elf32_Sym);
    memmove(&sym, cbytes + absoffset, sizeof(sym));
    if (sym.st_info == STT_FUNC || sym.st_info == 18) 
    {
      char* name = cbytes + strtab_off + sym.st_name;
      char* dynname = cbytes + dynstr_off + sym.st_name;
      if (strlen(name)) {
        strcpy(sts[sts_idx].name, name);
      } else {
        strcpy(sts[sts_idx].name, dynname);
      }
      sts[sts_idx].addr = sym.st_value;
      sts[sts_idx].size = sym.st_size;
      ++sts_idx;
    }
  }
  return sts_idx;
}

char* find_symbol(word_t addr) {
  for (int i = 0; i < sts_len; ++i) {
    if (addr >= elfsts[i].addr && addr < elfsts[i].addr + elfsts[i].size) {
      return elfsts[i].name;
    }
  }
  return NULL;
}

void init_elf(const char* elf_file) {
  if (elf_file == NULL) {
    perror("no input elf file!\n");
    return;
  }
  Log("Parsing elf: %s\n", elf_file);
  void *pybytes = mmapfile(elf_file);
  sts_len = parseelf(pybytes, elfsts);
}
