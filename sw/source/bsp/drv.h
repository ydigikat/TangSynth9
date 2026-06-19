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

void trace_str(TRACE_t *restrict trace, const char *s);
void trace_str_dec(TRACE_t *restrict trace, const char *s, uint32_t val);
void trace_str_hex(TRACE_t *restrict trace, const char *s, uint32_t val, uint8_t digits);

#ifdef TRACE_ENABLED

static inline void trace_init(TRACE_t *restrict trace)
{
  MODIFY_REG(trace->CR, TRACE_CR_DIV, _VAL2FLD(TRACE_CR_DIV, TRACE_DIV));
}

#define TRACE_ASSERT(expr) \
  do { \
    if (!(expr)) { \
      trace_str(TRACE, TRACE_FILE); \
      trace_str_dec(TRACE, "  line", __LINE__); \
      while (1); \
    } \
  } while (0)

#define TRACE_PRINT(s) trace_str(TRACE, s)
#define TRACE_PRINT_DEC(s, v) trace_str_dec(TRACE, s, v)
#define TRACE_PRINT_HEX(s, v, d) trace_str_hex(TRACE, s, v, d)
#else
#define TRACE_ASSERT(expr)
#define TRACE_PRINT(s)
#define TRACE_PRINT_DEC(s, v)
#define TRACE_PRINT_HEX(s, v, d)
#endif



/*----------------------------------------------------------------------------
 * MIDI Driver
 * --------------------------------------------------------------------------*/

static inline void midi_init(MIDI_t *restrict midi)
{
  MODIFY_REG(midi->CR, MIDI_CR_DIV, _VAL2FLD(MIDI_CR_DIV, MIDI_DIV));
}

static inline bool midi_rx_ready(MIDI_t *restrict midi)
{
  return READ_BIT(midi->SR, MIDI_SR_RXRDY);
}

static inline bool midi_rx_error(MIDI_t *restrict midi)
{
  return READ_BIT(midi->SR, MIDI_SR_ERR);
}

static inline uint8_t midi_read_byte(MIDI_t *restrict midi)
{
  return (uint8_t)_FLD2VAL(MIDI_RD_DAT, READ_REG(midi->RD));
}

#endif