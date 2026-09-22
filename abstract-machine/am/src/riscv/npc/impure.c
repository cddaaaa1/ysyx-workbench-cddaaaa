#include <stdint.h>

/* newlib 兼容: 程序里的 stdout/stderr/stdin 宏会展开为
 * _impure_ptr->_stdout 等成员访问, 这里补上 newlib 的 reentrancy 指针 */
struct _reent {
  int _errno;
  void *_stdin;
  void *_stdout;
  void *_stderr;
};

/* klib 提供的三个标准流 FILE 对象 */
extern char stdin, stdout, stderr;

static struct _reent __reent = {0, &stdin, &stdout, &stderr};

struct _reent *_impure_ptr  = &__reent;
struct _reent *__impure_ptr = &__reent;
