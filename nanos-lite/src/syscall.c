#include <common.h>
#include "syscall.h"
#include "fs.h"

const char* sys_names[] = {
  "SYS_exit",
  "SYS_yield",
  "SYS_open",
  "SYS_read",
  "SYS_write",
  "SYS_kill",
  "SYS_getpid",
  "SYS_close",
  "SYS_brk",
  "SYS_lseek",
  "SYS_fstat",
  "SYS_time",
  "SYS_signal",
  "SYS_execve",
  "SYS_fork",
  "SYS_link",
  "SYS_unlink",
  "SYS_wait",
  "SYS_times",
  "SYS_gettimeofday",
};

extern char _end;

int sys_write(int fd, const void* buf, size_t len) {
  const char* bf = buf;
  if (fd == 1 || fd == 2) {
    int i;
    for (i = 0; i < len; ++i) {
      putch(bf[i]);
    }
    return i;
  } else {
    return fs_write(fd, buf, len);
  }
}

int sys_brk(void* addr) {
  return 0;
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
    case SYS_open:
      c->GPRx = fs_open((const char*)a[1], a[2], a[3]);
      break;
    case SYS_read:
      c->GPRx = fs_read(a[1], (void*)a[2], a[3]);
      break;
    case SYS_write: 
      c->GPRx = sys_write(a[1], (const void*)a[2], a[3]); 
      break;
    case SYS_close:
      c->GPRx = fs_close(a[1]);
    case SYS_brk:
      c->GPRx = sys_brk((void*)a[1]);
      break;
    case SYS_lseek:
      c->GPRx = fs_lseek(a[1], a[2], a[3]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#ifdef CONFIG_STRACE
  Log("STRACE SYSCALL: %s(%d, %d, %d) -> %d", sys_names[a[0]], a[1], a[2], a[3], c->GPRx);
#endif
}
