/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __DRV_H__
#define __DRV_H__

#include "bsp.h"

/* ----------------------------------------------------------------------------
 * GPO Driver
 * --------------------------------------------------------------------------*/
static inline void gpo_set_pin(GPO_t *restrict gpo, uint32_t pin_mask)
{
  WRITE_REG(gpo->BSR, pin_mask);
}

static inline void gpo_clear_pin(GPO_t *restrict gpo, uint32_t pin_mask)
{
  WRITE_REG(gpo->BSR, pin_mask << 16U);
}

/* ----------------------------------------------------------------------------
 * Trace Driver
 * --------------------------------------------------------------------------*/
void trace_print(TRACE_t *restrict trace, const char *str);
void trace_printf(TRACE_t *restrict trace, const char *fmt, uint32_t arg1, uint32_t arg2, uint32_t arg3);

static inline void trace_set_divisor(TRACE_t *restrict trace, uint16_t div)
{
  MODIFY_REG(trace->CR, TRACE_CR_DIV, _VAL2FLD(TRACE_CR_DIV, div));
}

static inline void trace_init(TRACE_t *restrict trace)
{
  trace_set_divisor(trace, TRACE_DIV);
}

static inline bool trace_send_data_ready(TRACE_t *restrict trace)
{
  return READ_BIT(trace->SR, TRACE_SR_TXRDY);
}

static inline uint8_t trace_putch(TRACE_t *restrict trace, uint8_t c)
{
  uint16_t timeout_count = TRACE_TIMEOUT_COUNT;

  WRITE_REG(trace->TD, c);

  while (!trace_send_data_ready(trace))
  {
    if (timeout_count-- == 0)
    {
      return TRACE_TIMEOUT;
    }
  }

  return TRACE_SUCCESS;
}

#endif