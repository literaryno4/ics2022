/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

// start of the audio stream buffer ring
static uint8_t *sbuf_start;
static int inited_audio = false;

static void audio_play(void*userdata, uint8_t* stream, int len) {
  int nread = len;
  int count = audio_base[5];
  if (count == 0) return;
  if (count < len) {
    nread = count;
  }
  int b = 0;
  while (b < nread) {
    stream[b] = *sbuf_start;
    sbuf_start = sbuf + ((sbuf_start - sbuf + 1) % CONFIG_SB_SIZE);
    ++b;
  }
  audio_base[5] -= nread;
  if (len > nread) {
    memset(stream + nread, 0, len - nread);
  }
}


static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if (inited_audio || audio_base[5] == 0) {
    return;
  }
  inited_audio = true;
  SDL_AudioSpec s;

  SDL_zero(s);
  s.format = AUDIO_S16SYS;
  s.userdata = NULL;
  s.freq = audio_base[0];
  s.channels = audio_base[1]; 
  s.samples = audio_base[2];
  s.callback = audio_play;

  SDL_InitSubSystem(SDL_INIT_AUDIO);
  SDL_OpenAudio(&s, NULL);
  SDL_PauseAudio(0);
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  sbuf_start = sbuf;
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
