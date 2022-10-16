#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) { 
  char buffer[2048];
  va_list arg;
  va_start(arg, fmt);
  int ret = vsprintf(buffer, fmt, arg);
  putstr(buffer);
  va_end(arg);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, -1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  int ret = vsprintf(out, fmt, arg);
  va_end(arg);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  int ret = vsnprintf(out, n, fmt, arg);
  va_end(arg);
  return ret;
}

#define output(x) {out[j++] = (x); if (j >=n) {break;}}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  va_list ap2;
  long long d;
  int islong = 0;
  int zero_pad = 0;
  char ch, *s;
  va_copy(ap2, ap);
  int j = 0;
  while ((ch = *fmt++) && j < n) {
    if ('%' == ch) {
      if(*fmt == '0') {
        while ((ch = *fmt++) && (ch >= '0' && ch <= '9')) {
          zero_pad = zero_pad * 10 + (ch - '0');
        }
        --fmt;
      }
      if (*fmt == 'l') {
        islong = 1;
        ++fmt;
      }
      switch (ch = *fmt++) {
        case 'c':
          ch = va_arg(ap, int);
          output(ch);
          break;
        case 's':
          s = va_arg(ap, char *);
          for (int k = 0; s[k] != '\0'; ++k) {
            output(s[k]);
          }
          break;
        case 'p':
          output('0');
          output('x');
        case 'X':
        case 'x': {
          char buffer[64];
          const char* hexch = (ch == 'X' ? "0123456789ABCDEF" : "0123456789abcdef");
          assert(strlen(hexch) == 16);
          int i = 0;
          if (islong) {
            d = va_arg(ap, long long);
            islong = 0;
          } else {
            d = va_arg(ap, int);
          }
          if (d < 0) {
            output('-');
            d = -d;
          }
          if (d == 0) {
            buffer[i++] = '0';
          }
          while (d > 0) {
            buffer[i++] = (hexch[d % 16]);
            d /= 16;
          }
          while (zero_pad > i) {
            output('0');
            --zero_pad;
          }
          zero_pad = 0;
          while (i > 0) {
            output(buffer[--i]);
          }
        } break;

        case 'd': {
          char buffer[64];
          int i = 0;
          if (islong) {
            d = va_arg(ap, long long);
            islong = 0;
          } else {
            d = va_arg(ap, int);
          }
          if (d < 0) {
            output('-');
            d = -d;
          }
          if (d == 0) {
            buffer[i++] = '0';
          }
          while (d > 0) {
            buffer[i++] = (d % 10 + '0');
            d /= 10;
          }
          while (zero_pad > i) {
            output('0');
            --zero_pad;
          }
          zero_pad = 0;
          while (i > 0) {
            output(buffer[--i]);
          }
        } break;

        default:
          break;
      }
    } else {
      output(ch);
    }
  }
  
  out[j] = '\0';
  assert(j <= n);
  return j;
}

#endif
