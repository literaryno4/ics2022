#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t fbctl_write(const void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENTS, FD_FBCTL, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write},
  [FD_FBCTL] = {"/dev/fbctl", 0, 0, invalid_read, fbctl_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
#include "files.h"
};

#define FILE_NUM sizeof(file_table) / sizeof(Finfo)

typedef struct {
  bool opened;
  size_t cur_offset;
} open_file;

static open_file open_files[FILE_NUM];

void init_fs() {
  // TODO: initialize the size of /dev/fb
  open_files[FD_STDIN].opened = true;
  open_files[FD_STDOUT].opened = true;
  open_files[FD_STDERR].opened = true;
  open_files[FD_EVENTS].opened = true;
  open_files[FD_FBCTL].opened = true;
  open_files[FD_FB].opened = true;
}


int fs_open(const char *pathname, int flags, int mode) {
  int fd;
  for (fd = 0; fd < FILE_NUM; ++fd) {
    if (strcmp(pathname, file_table[fd].name) == 0) {
      open_files[fd].opened = true;
      open_files[fd].cur_offset = file_table[fd].disk_offset;
      return fd;
    }
  }
  panic("fs_open error!\n");
}

int fs_read(int fd, void *buf, size_t len) {
  if (!open_files[fd].opened) panic("fs_read error!\n");
  int read_len, ret;
  size_t (*read_fn) (void *buf, size_t offset, size_t len);
  read_fn = file_table[fd].read;
  if (read_fn == NULL) read_fn = ramdisk_read;
  if (file_table[fd].size == 0) {
    return read_fn(buf, open_files[fd].cur_offset, len);
  }
  if (open_files[fd].cur_offset >= file_table[fd].disk_offset + file_table[fd].size) {
    memset(buf, '\0', len);
    return len;
  } else if (open_files[fd].cur_offset + len >= file_table[fd].disk_offset + file_table[fd].size) {
    read_len = file_table[fd].disk_offset + file_table[fd].size - open_files[fd].cur_offset;
    ret = read_fn(buf, open_files[fd].cur_offset, read_len);
    memset(buf + read_len, '\0', len - read_len);
  } else {
    ret = read_fn(buf, open_files[fd].cur_offset, len);
  }
  open_files[fd].cur_offset += ret;
  return ret;
}

int fs_write(int fd, const void *buf, size_t len) {
  if (!open_files[fd].opened) panic("fs_write error!\n");
  // assert(open_files[fd].cur_offset + len < file_table[fd].disk_offset + file_table[fd].size);
  size_t (*write_fn) (const void *buf, size_t offset, size_t len);
  write_fn = file_table[fd].write == NULL ? ramdisk_write : file_table[fd].write;

  int ret = write_fn(buf, open_files[fd].cur_offset, len);
  open_files[fd].cur_offset += ret;
  return ret;
}

int fs_lseek(int fd, size_t offset, int whence) {
  if (!open_files[fd].opened) panic("fs_ls error!\n");
  switch (whence) {
  case SEEK_SET:
    open_files[fd].cur_offset = file_table[fd].disk_offset + offset;
    break;
  case SEEK_CUR:
    open_files[fd].cur_offset += offset;
    break;
  case SEEK_END:
    open_files[fd].cur_offset = file_table[fd].disk_offset + file_table[fd].size + offset;
    break;
  default:
    panic("fs_lseek error: no such whence\n");
  }
  return open_files[fd].cur_offset - file_table[fd].disk_offset;
}

int fs_close(int fd) {
  open_files[fd].opened = false;
  open_files[fd].cur_offset = file_table[fd].disk_offset;
  return 0;
}
