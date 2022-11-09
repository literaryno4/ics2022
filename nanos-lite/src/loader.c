#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr elf_header;
  memcpy(&elf_header, &ramdisk_start, sizeof(Elf32_Ehdr));
  assert(*(uint32_t*)elf_header.e_ident == 0x464c457f);
  assert(elf_header.e_phnum < 8);

  // check architecture
#ifdef __LP64__
  assert(elf_header.e_ident[EI_CLASS] == ELFCLASS64);
#else 
  assert(elf_header.e_ident[EI_CLASS] == ELFCLASS32);
#endif
#if defined(__ISA_AM_NATIVE__)
  assert(elf_header.e_machine == EM_X86_64);
#elif defined(__ISA_X86__)
  assert(elf_header.e_machine == EM_X86_64);
#elif defined(__ISA_RISCV32__)
  assert(elf_header.e_machine == EM_RISCV);
#elif defined(__ISA_RISCV64__)
  assert(elf_header.e_machine == EM_RISCV);
#else
# error Unsupported ISA
#endif

  // load segment to memory
  Elf_Phdr program_headers[8]; // segment
  memcpy(&program_headers, &ramdisk_start + elf_header.e_phoff, sizeof(Elf32_Phdr) * elf_header.e_phnum);
  for (int i = 0; i < elf_header.e_phnum; ++i) {
    if (program_headers[i].p_type == PT_LOAD) {
      memcpy((void*)(program_headers[i].p_vaddr), &ramdisk_start + program_headers[i].p_offset, program_headers[i].p_filesz);
      memset((void*)(program_headers[i].p_vaddr + program_headers[i].p_filesz), 0, program_headers[i].p_memsz - program_headers[i].p_filesz);
    }
  }

  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

