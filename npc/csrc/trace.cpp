#include "include/trace.h"

#ifdef ITRACE

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <capstone/capstone.h>
#include "include/macro.h"

static FILE *log_fp = nullptr;

static void init_log(const char *log_file) {
  log_fp = stdout;
  if (log_file != nullptr) {
    FILE *fp = fopen(log_file, "w");
    assert(fp != nullptr);
    log_fp = fp;
  }
  printf("Log is written to %s\n", log_file != nullptr ? log_file : "stdout");
}

#define log_write(...) \
  do { \
    if (log_fp != NULL) { \
      fprintf(log_fp, __VA_ARGS__); \
      fflush(log_fp); \
    } \
  } while (0)

static size_t (*cs_disasm_dl)(csh handle, const uint8_t *code,
    size_t code_size, uint64_t address, size_t count, cs_insn **insn);
static void (*cs_free_dl)(cs_insn *insn, size_t count);

static csh handle;

static void init_disasm() {
  void *dl_handle = dlopen(str(CAPSTONE_LIB), RTLD_LAZY);
  assert(dl_handle);

  cs_err (*cs_open_dl)(cs_arch arch, cs_mode mode, csh *handle) = nullptr;
  cs_open_dl = reinterpret_cast<decltype(cs_open_dl)>(dlsym(dl_handle, "cs_open"));
  assert(cs_open_dl);

  cs_disasm_dl = reinterpret_cast<decltype(cs_disasm_dl)>(dlsym(dl_handle, "cs_disasm"));
  assert(cs_disasm_dl);

  cs_free_dl = reinterpret_cast<decltype(cs_free_dl)>(dlsym(dl_handle, "cs_free"));
  assert(cs_free_dl);

  int ret = cs_open_dl(CS_ARCH_RISCV, (cs_mode)(CS_MODE_RISCV32 | CS_MODE_RISCVC), &handle);
  assert(ret == CS_ERR_OK);
}

static void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
  cs_insn *insn;
  size_t count = cs_disasm_dl(handle, code, nbyte, pc, 0, &insn);
  assert(count == 1);
  int ret = snprintf(str, size, "%s", insn->mnemonic);
  if (insn->op_str[0] != '\0') {
    snprintf(str + ret, size - ret, "\t%s", insn->op_str);
  }
  cs_free_dl(insn, count);
}

#define IRINGBUF_CAPACITY 16
#define IRINGBUF_LINE_SIZE 128

typedef struct {
  char records[IRINGBUF_CAPACITY][IRINGBUF_LINE_SIZE];
  size_t next;
  size_t count;
} IRingBuf;

static IRingBuf iringbuf_storage = {0};
static IRingBuf *b = &iringbuf_storage;

static void iringbuf_push(const char *record) {
  snprintf(b->records[b->next], IRINGBUF_LINE_SIZE, "%s", record);
  b->next = (b->next + 1) % IRINGBUF_CAPACITY;
  if (b->count < IRINGBUF_CAPACITY) {
    b->count ++;
  }
}

void iringbuf_dump() {
  if (b->count == 0) return;

  size_t start = (b->count == IRINGBUF_CAPACITY) ? b->next : 0;
  for (size_t i = 0; i < b->count; i ++) {
    size_t index = (start + i) % IRINGBUF_CAPACITY;
    printf("%s\n", b->records[index]);
  }
}

void init_trace(const char *log_file) {
  init_log(log_file);
  init_disasm();
}

void itrace(uint32_t pc, uint32_t inst, bool print_step) {
  char logbuf[128];
  char *p = logbuf;
  p += snprintf(p, sizeof(logbuf), "0x%08x:", pc);

  int ilen = 4;
  uint8_t *code = reinterpret_cast<uint8_t *>(&inst);
  for (int i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", code[i]);
  }

  int space_len = (4 - ilen) * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;
  disassemble(p, logbuf + sizeof(logbuf) - p, pc, code, ilen);

  iringbuf_push(logbuf);
  log_write("%s\n", logbuf);
  if (print_step) { puts(logbuf); }
}

#endif
