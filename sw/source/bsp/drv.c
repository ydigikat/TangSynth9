/*
 *  (c) Jason Wilden, 2026
 */
#include "drv.h"

/* ----------------------------------------------------------------------------
 * Trace Driver
 * --------------------------------------------------------------------------*/
#ifdef TRACE_ENABLED

static uint8_t trace_putch(TRACE_t *restrict trace, uint8_t c)
{
  uint16_t timeout_count = TRACE_TIMEOUT_COUNT;

  WRITE_REG(trace->TD, c);

  while (!READ_BIT(trace->SR, TRACE_SR_TXRDY))
  {
    if (timeout_count-- == 0)
    {
      return TRACE_TIMEOUT;
    }
  }

  return TRACE_SUCCESS;
}

static void putchar_blocking(TRACE_t *restrict trace, char c)
{
  trace_putch(trace, c);
}

static void print_str(TRACE_t *restrict trace, const char *s)
{
  while (*s)
  {
    if (*s == '\n')
      putchar_blocking(trace, '\r');
    putchar_blocking(trace, *s++);
  }
}

static void print_hex(TRACE_t *restrict trace, uint32_t val, uint8_t digits)
{
  static const char hex[] = "0123456789ABCDEF";
  for (int i = (digits - 1) * 4; i >= 0; i -= 4)
    putchar_blocking(trace, hex[(val >> i) & 0xF]);
}

static void print_dec(TRACE_t *restrict trace, uint32_t val)
{
  char buf[10];
  int i = 0;

  if (val == 0)
  {
    putchar_blocking(trace, '0');
  }
  else
  {
    while (val)
    {
      buf[i++] = '0' + (val % 10);
      val /= 10;
    }
    while (i > 0)
      putchar_blocking(trace, buf[--i]);
  }
}

void trace_str(TRACE_t *restrict trace, const char *s)
{
  print_str(trace, s);
  putchar_blocking(trace, '\n');
  putchar_blocking(trace, '\r');
}

void trace_str_dec(TRACE_t *restrict trace, const char *s, uint32_t val)
{
  print_str(trace, s);
  putchar_blocking(trace, ' ');
  print_dec(trace, val);
  putchar_blocking(trace, '\n');
  putchar_blocking(trace, '\r');
}

void trace_str_hex(TRACE_t *restrict trace, const char *s, uint32_t val, uint8_t digits)
{
  print_str(trace, s);
  putchar_blocking(trace, ' ');
  putchar_blocking(trace, '0');
  putchar_blocking(trace, 'x');
  print_hex(trace, val, digits);
  putchar_blocking(trace, '\n');
  putchar_blocking(trace, '\r');
}

#endif
