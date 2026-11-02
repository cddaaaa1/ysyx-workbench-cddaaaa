#include <cpu/ftrace.h>
#include <elf.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>

#ifdef CONFIG_FTRACE
#define MAX_FUNCTIONS 128

typedef struct {
  uint64_t addr;
  uint64_t size;
  char name[256];
} Function;

static Function functions[MAX_FUNCTIONS];
static size_t function_count = 0;
static int depth = 0;

static bool read_at(FILE *fp, uint64_t file_size, uint64_t offset,
                    void *buffer, size_t size) {
  if (offset > file_size || size > file_size - offset || offset > LONG_MAX) {
    fprintf(stderr, "ELF data is outside the file\n");
    return false;
  }
  if (fseek(fp, (long)offset, SEEK_SET) != 0 ||
      fread(buffer, 1, size, fp) != size) {
    fprintf(stderr, "Cannot read ELF data\n");
    return false;
  }
  return true;
}

bool init_ftrace(const char *ftrace_file) {
  function_count = 0;
  depth = 0;
  bool success = false;
  FILE *fp = NULL;
  Elf32_Shdr *sections = NULL;
  char *strtab = NULL;

  if (ftrace_file == NULL || ftrace_file[0] == '\0') {
    fprintf(stderr, "Missing ELF file for ftrace\n");
    goto cleanup;
  }

  fp = fopen(ftrace_file, "rb");
  if (fp == NULL) {
    perror("Cannot open ELF file");
    goto cleanup;
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "Cannot seek ELF file\n");
    goto cleanup;
  }
  long file_size = ftell(fp);
  if (file_size < 0) {
    fprintf(stderr, "Cannot get ELF file size\n");
    goto cleanup;
  }

  Elf32_Ehdr ehdr;
  if (!read_at(fp, file_size, 0, &ehdr, sizeof(ehdr))) goto cleanup;

  const uint16_t host_byte_order = 1;
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr.e_ident[EI_CLASS] != ELFCLASS32 ||
      ehdr.e_ident[EI_DATA] != ELFDATA2LSB ||
      *(const unsigned char *)&host_byte_order != 1 ||
      ehdr.e_ident[EI_VERSION] != EV_CURRENT ||
      ehdr.e_version != EV_CURRENT ||
      ehdr.e_machine != EM_RISCV || ehdr.e_type != ET_EXEC ||
      ehdr.e_ehsize != sizeof(Elf32_Ehdr)) {
    fprintf(stderr, "Expected ELF32 little-endian RISC-V executable on a little-endian host\n");
    goto cleanup;
  }

  if (ehdr.e_shoff == 0 || ehdr.e_shnum == 0 ||
      ehdr.e_shentsize != sizeof(Elf32_Shdr)) {
    fprintf(stderr, "Missing or unsupported ELF section header table\n");
    goto cleanup;
  }

  size_t sections_size = (size_t)ehdr.e_shnum * sizeof(Elf32_Shdr);
  sections = malloc(sections_size);
  if (sections == NULL) {
    fprintf(stderr, "Cannot allocate ELF section headers\n");
    goto cleanup;
  }
  if (!read_at(fp, file_size, ehdr.e_shoff, sections, sections_size)) goto cleanup;

  const Elf32_Shdr *symtab = NULL;
  for (size_t i = 0; i < ehdr.e_shnum; i ++) {
    if (sections[i].sh_type == SHT_SYMTAB) {
      symtab = &sections[i];
      break;
    }
  }
  if (symtab == NULL) {
    fprintf(stderr, "ELF has no symbol table\n");
    goto cleanup;
  }
  if (symtab->sh_entsize != sizeof(Elf32_Sym) ||
      symtab->sh_size % sizeof(Elf32_Sym) != 0 ||
      symtab->sh_link >= ehdr.e_shnum ||
      (uint64_t)symtab->sh_offset + symtab->sh_size > (uint64_t)file_size) {
    fprintf(stderr, "Invalid ELF symbol table\n");
    goto cleanup;
  }

  const Elf32_Shdr *strings = &sections[symtab->sh_link];
  if (strings->sh_type != SHT_STRTAB || strings->sh_size == 0 ||
      (uint64_t)strings->sh_offset + strings->sh_size > (uint64_t)file_size) {
    fprintf(stderr, "Invalid ELF symbol string table\n");
    goto cleanup;
  }
  strtab = malloc(strings->sh_size);
  if (strtab == NULL) {
    fprintf(stderr, "Cannot allocate ELF string table\n");
    goto cleanup;
  }
  if (!read_at(fp, file_size, strings->sh_offset, strtab, strings->sh_size)) goto cleanup;

  size_t symbol_count = symtab->sh_size / symtab->sh_entsize;
  for (size_t i = 0; i < symbol_count; i ++) {
    Elf32_Sym sym;
    uint64_t offset = (uint64_t)symtab->sh_offset + i * symtab->sh_entsize;
    if (!read_at(fp, file_size, offset, &sym, sizeof(sym))) goto cleanup;
    if (ELF32_ST_TYPE(sym.st_info) != STT_FUNC || sym.st_shndx == SHN_UNDEF) continue;

    if ((sym.st_shndx >= ehdr.e_shnum && sym.st_shndx != SHN_ABS) ||
        sym.st_name >= strings->sh_size) {
      fprintf(stderr, "Invalid or unsupported ELF function symbol\n");
      goto cleanup;
    }

    const char *name = strtab + sym.st_name;
    const char *end = memchr(name, '\0', strings->sh_size - sym.st_name);
    if (end == NULL || (size_t)(end - name) >= sizeof(functions[0].name)) {
      fprintf(stderr, "Invalid or too long ELF function name\n");
      goto cleanup;
    }
    if (name[0] == '\0') continue;
    if (function_count == MAX_FUNCTIONS) {
      fprintf(stderr, "Exceed max functions for ftrace\n");
      goto cleanup;
    }

    Function *func = &functions[function_count];
    func->addr = sym.st_value;
    func->size = sym.st_size;
    memcpy(func->name, name, (size_t)(end - name) + 1);
    function_count ++;
  }

  if (function_count == 0) {
    fprintf(stderr, "No defined function symbols found\n");
    goto cleanup;
  }
  success = true;

cleanup:
  free(strtab);
  free(sections);
  if (fp != NULL && fclose(fp) != 0) {
    fprintf(stderr, "Cannot close ELF file\n");
    success = false;
  }
  if (!success) function_count = 0;
  Assert(success, "Failed to initialize ftrace");
  return success;
}

const char *ftrace_find_function_name(uint64_t addr) {
  for (size_t i = 0; i < function_count; i ++) {
    const Function *func = &functions[i];
    if (addr == func->addr || (addr > func->addr && addr - func->addr < func->size)) {
        return func->name;
    }
  }
  return NULL;
}

void ftrace_call(uint64_t pc, uint64_t target){
  const char *func_name = ftrace_find_function_name(target);
  if (func_name == NULL) {
    func_name = "unknown";
    Log("ftrace: unknown function called at pc = 0x%08" PRIx64 ", target = 0x%08" PRIx64, pc, target);
  }
  printf("0x%08" PRIx64 ": %*scall [%s@0x%08" PRIx64 "]\n",
      pc, depth * 2, "",func_name, target);
  depth ++;
}

void ftrace_ret(uint64_t pc, uint64_t target){
  const char *func_name = ftrace_find_function_name(pc);
  if (func_name == NULL) {
    func_name = "unknown";
    Log("ftrace: unknown function returned at pc = 0x%08" PRIx64 ", target = 0x%08" PRIx64, pc, target);
  }
  if (depth > 0) {
    depth--;
  }
  printf("0x%08" PRIx64 ": %*sret  [%s]\n",
    pc, depth * 2, "", func_name);
}

#endif
