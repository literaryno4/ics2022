#include <am.h>
#include <nemu.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

#define SBUF_SIZE 0x10000

static uintptr_t sb_pos;

void __am_audio_init() {
  sb_pos = AUDIO_SBUF_ADDR;
  outl(AUDIO_COUNT_ADDR, 0);
  for (int i = 0; i < SBUF_SIZE; ++i) {
    outb(AUDIO_SBUF_ADDR + i, 0);
  }
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = SBUF_SIZE;
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
  uintptr_t start = (uintptr_t)ctl->buf.start;
  uintptr_t end = (uintptr_t)ctl->buf.end;
  uint32_t len = end - start;
  int count = inl(AUDIO_COUNT_ADDR);
  while (count + len > SBUF_SIZE);
  for (int i = 0; i < len; i++) {
    outb(sb_pos, ((uint8_t*)start)[i]);
    sb_pos = AUDIO_SBUF_ADDR + ((sb_pos - AUDIO_SBUF_ADDR + 1) % SBUF_SIZE);
  }
  outl(AUDIO_COUNT_ADDR, count + len);
}
