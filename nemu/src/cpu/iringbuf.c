#include <common.h>
#include <cpu/iringbuf.h>

#ifdef CONFIG_ITRACE
#define IRINGBUF_CAPACITY 16
#define IRINGBUF_LINE_SIZE 128

typedef struct {
  char records[IRINGBUF_CAPACITY][IRINGBUF_LINE_SIZE];
  size_t next;
  size_t count;
} IRingBuf;

static IRingBuf iringbuf_storage = {0};
static IRingBuf *b = &iringbuf_storage;

void iringbuf_push(const char *record) {
  snprintf(b->records[b->next], IRINGBUF_LINE_SIZE, "%s", record);
  b->next = (b->next + 1) % IRINGBUF_CAPACITY;
  if (b->count < IRINGBUF_CAPACITY) {
    b->count ++;
  }
}

void iringbuf_dump(void) {
  if (b->count == 0) return;

  size_t start = (b->count == IRINGBUF_CAPACITY) ? b->next : 0;
  for (size_t i = 0; i < b->count; i ++) {
    size_t index = (start + i) % IRINGBUF_CAPACITY;
    printf("%s\n", b->records[index]);
  }
}
#endif
