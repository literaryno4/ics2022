#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <assert.h>
#include "NDL.h"

int main() {
  struct timeval tv;
  uint32_t start = NDL_GetTicks();
  uint32_t interval = 0;
  while (1) {
    uint32_t t = NDL_GetTicks();
    if (t - start >= interval) {
      printf("a sentence after %ds\n", interval / 1000);
      interval += 500;
    }
  }

  printf("PASS!!!\n");

  return 0;
}
