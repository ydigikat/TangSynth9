/*
 *  (c) Jason Wilden, 2026
 */
#include "drv.h"

/* ----------------------------------------------------------------------------
 * Trace Driver
 * --------------------------------------------------------------------------*/

static inline void putchar_blocking(TRACE_t *restrict trace, char c)
{
  trace_putch(trace, c);
}

static void print_str(TRACE_t *restrict trace, const char *s)
{
  if (s == NULL)
    return;

  while (*s)
  {
    if (*s == '\n')
      putchar_blocking(trace, '\r');
    putchar_blocking(trace, *s++);
  }
}

static void print_hex(TRACE_t *restrict trace, uint32_t val, uint8_t digits)
{
  const char hex[] = "0123456789ABCDEF";
  for (int i = (digits - 1) * 4; i >= 0; i -= 4)
  {
    putchar_blocking(trace, hex[(val >> i) & 0xF]);
  }
}

static void print_dec(TRACE_t *restrict trace, uint32_t val)
{
  if (val == 0)
  {
    putchar_blocking(trace, '0');
    return;
  }

  char buf[10];
  int i = 0;
  while (val)
  {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0)
    putchar_blocking(trace, buf[--i]);
}

void trace_print(TRACE_t *restrict trace, const char *str)
{
  trace_printf(trace, str, 0, 0, 0);
}

void trace_printf(TRACE_t *restrict trace, const char *fmt, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
  uint32_t args[3] = {arg1, arg2, arg3};
  int arg_idx = 0;

  while (*fmt)
  {
    if (*fmt == '%' && arg_idx < 3)
    {
      fmt++;
      switch (*fmt)
      {
      case 'd':
        print_dec(trace, args[arg_idx++]);
        break;
      case 'x':
        print_hex(trace, args[arg_idx++], 8);
        break;
      case 'h':
        print_hex(trace, args[arg_idx++], 4);
        break; // Short hex
      case 'b':
        print_hex(trace, args[arg_idx++], 2);
        break; // Byte hex
      case 's':
        print_str(trace, (const char *)args[arg_idx++]);
        break;
      case 'c':
        putchar_blocking(trace, args[arg_idx++] & 0xFF);
        break;
      default:
        putchar_blocking(trace, *fmt);
      }
    }
    else
    {
      if (*fmt == '\n')
        putchar_blocking(trace, '\r');
      putchar_blocking(trace, *fmt);
    }
    fmt++;
  }
}