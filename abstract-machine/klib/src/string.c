#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

typedef unsigned char uchar;

size_t strlen(const char *s) {
  size_t n;
  for (n = 0; s[n]; n++);
  return n;
}

char *strcpy(char *dst, const char *src) {
  size_t i;
  for (i = 0; src[i] != '\0'; i++)
     dst[i] = src[i];
  dst[i] = '\0';

  return dst;
}

char *strncpy(char *s, const char *t, size_t size) {
  char* os;
  os = s;
  int n = size;
  while (n-- > 0 && (*s++ = *t++) != 0); 
  while (n-- > 0) {
    *s++ = 0;
  }
  return os;
}

char *strcat(char *dst, const char *src) {
  size_t dst_len = strlen(dst);
  size_t i;

  for (i = 0 ; src[i] != '\0' ; i++)
     dst[dst_len + i] = src[i];
  dst[dst_len + i] = '\0';

  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t i;
  size_t len = strlen(s1);
  for (i = 0; i < len && s1[i] == s2[i]; ++i);
  return s1[i] - s2[i];
}

int strncmp(const char *p, const char *q, size_t size) {
  int n = size;
  while (n > 0 && *p && *p == *q) {
    n--, p++, q++;
  }
  if (n == 0) {
    return 0;
  }
  return (uchar)*p - (uchar)*q;
}

void *memset(void *s, int c, size_t size) {
  unsigned char* ret = (unsigned char*)s;
  int n = size;
  while (n--) {
    *ret = c;
    ++ret;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t size) {
  const char* s;
  char *d;
  int n = size;
  s = src;
  d = dst;
  if (s < d && s + n > d) {
    s += n;
    d += n;
    while (n-- > 0) {
      *--d = *--s;
    }   
  } else {
    while (n-- > 0) {
      *d++ = *s++;
    }
  }
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  return memmove(dst, src, n);
}

int memcmp(const void *v1, const void *v2, size_t size) {
  const uchar *s1, *s2;
  int n = size;
  s1 = v1;
  s2 = v2;

  while (n-- > 0) {
    if (*s1 != *s2) {
      return *s1 - *s2;
    }
    s1++, s2++;
  }

  return 0;
}

#endif
