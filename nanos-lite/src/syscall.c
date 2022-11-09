#include <common.h>
#include "syscall.h"

int sys_write(int fd, const void* buf, size_t len) {
  const char* bf = buf;
  if (fd == 1 || fd == 2) {
    int i;
    for (i = 0; i < len; ++i) {
      putch(bf[i]);
    }
    return i;
  } else {
    panic("write file not support\n");
  }
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case SYS_exit: halt(a[1]); break;
    case SYS_yield: yield(); c->GPRx = 0; break;
    case SYS_write: 
      c->GPRx = sys_write(a[1], (const void*)a[2], a[3]); 
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#ifdef CONFIG_STRACE
  Log("STRACE SYSCALL: %s(%d, %d, %d) -> %d\n", sys_names[a[0]].sysname, c->GPR2, c->GPR3, c->GPR4, c->GPRx);
#endif

}
