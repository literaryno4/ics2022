#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (int i = 0; i < len; ++i) {
    putch(((const char*)buf)[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) return 0;
  size_t nl = strlen(keyname[ev.keycode]);
  nl = len < nl ? len : nl;
  strncpy(buf, keyname[ev.keycode], nl);
  char* status = ev.keydown ? " DOWN" : " UP";
  strncpy(buf + nl, status, strlen(status) + 1);
  return nl + strlen(status);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  printf("in fb write\n");
  int w = 128, h = 128;
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      io_write(AM_GPU_FBDRAW, i, j, (void*)buf + (j * w + i), 1, 1, false);
    }
    io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true);
  }

  io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true);
  return len;
}

size_t fbctl_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
