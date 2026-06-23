/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __LFO_H__
#define __LFO_H__

#include <stdint.h>
#include <stdbool.h>

#include "luts.h"
#include "params.h"

/*
 * Phase runs as a uint16_t from 0x0000 to 0xFFFF.  Natural unsigned overflow.
 * One full cycle = 65536 counts.
 *
 * Inc is the per-block phase step (not per-sample), computed at update time
 * from a precomputed LUT:
 *
 *   inc = round(rate_hz * AUDIO_BLOCK_SIZE / SAMPLE_RATE * 65536)
 *
 * At max rate (20 Hz, block size 32, Fs 46875):
 *   inc = round(20 * 32 / 46875 * 65536) = 446  -- comfortably fits uint16_t
 *
 * Waveform outputs are signed Q1.15: -32768 (−1.0) to +32767 (+1.0).
 */

struct lfo
{
  /* Params */
  uint8_t rate; /* index into lfo_inc_lut  */
  enum lfo_mode mode;

  /* State */
  uint16_t phase; /* unsigned 16-bit phase accumulator, wraps  */
  uint16_t prev_phase;
  uint16_t inc; /* per-block phase step from lfo_inc_lut  */

  /* S&H state */
  int16_t sh_value; /* Q1.15, held output */
};

/* API */
void lfo_init(struct lfo *lfo);
void lfo_reset(struct lfo *lfo);
void lfo_render(struct lfo *lfo, SQ1_15 *tri, SQ1_15 *saw,
                SQ1_15 *ramp, SQ1_15 *square,
                SQ1_15 *sandh);
void lfo_update(struct lfo *lfo, uint8_t rate, enum lfo_mode mode);
void lfo_note_on(struct lfo *lfo);

#endif /* __LFO_H__*/