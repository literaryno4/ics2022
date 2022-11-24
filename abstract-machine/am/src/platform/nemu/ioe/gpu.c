#include <am.h>
#include <nemu.h>
#include <stdio.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  int i;
  int w = io_read(AM_GPU_CONFIG).width;
  int h = io_read(AM_GPU_CONFIG).height;
  uint32_t *fb = (uint32_t*)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i++) {
    fb[i] = 0;
  }
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = 400, .height = 300,
    .vmemsz = 400*300*4
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  int W = 400;
  uint32_t *pixels = (uint32_t*)ctl->pixels;
  uint32_t *fb = (uint32_t*)(uintptr_t)FB_ADDR;
  int pi = 0;
  for (int j = y; j < y + h; j++) {
    for (int i = x; i < x + w; i++, pi++) {
      fb[j * W + i] = pixels[pi];
    }
  }
    
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
  
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
