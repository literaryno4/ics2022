#include <am.h>
#include <nemu.h>
#include <string.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

static uint8_t* sb_addr = (uint8_t*)AUDIO_SBUF_ADDR;
static uint8_t* sb_pos = (uint8_t*)AUDIO_SBUF_ADDR;
static int sb_size;

void __am_audio_init() {
  outl(AUDIO_COUNT_ADDR, 0);
  sb_size = inl(AUDIO_SBUF_SIZE_ADDR);
  memset((void*)AUDIO_SBUF_ADDR, 0, sb_size);
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = sb_size;
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  uint8_t* start = (uint8_t*)(uintptr_t)ctl->buf.start;
  uint8_t* end = (uint8_t*)(uintptr_t)ctl->buf.end;
  uint32_t len = end - start;
  while ((inl(AUDIO_COUNT_ADDR) + len) > sb_size);
  for (int i = 0; i < len; i++) {
    *sb_pos = start[i];
    sb_pos = sb_addr + ((sb_pos - sb_addr + 1) % sb_size);
  }
  int count = inl(AUDIO_COUNT_ADDR);
  outl(AUDIO_COUNT_ADDR, count + len);
}
