/*
 *  (c) Jason Wilden, 2026
 */
#include "lfo.h"
#include "drv.h"

#ifdef TRACE_ENABLED
static const char *TRACE_FILE = "lfo.c";
#endif

static inline SQ1_15 lfo_triangle(uint16_t phase)
{
  /*
   * Fold the second half back: rising 0→+1 first half, falling +1→-1 second.
   * Result range: 0x0000 to 0x7FFF mapped to -0x7FFF to +0x7FFF, i.e. Q1.15.
   */
  uint16_t folded = (phase < 0x8000u) ? phase : (uint16_t)(0xFFFFu - phase);
  /* folded is 0..0x7FFF; scale to signed: out = folded * 2 - 0x7FFF */
  return (SQ1_15)((int32_t)folded * 2 - 0x7FFF);
}

static inline SQ1_15 lfo_saw(uint16_t phase)
{
  /*
   * Rising sawtooth: -1.0 at phase=0 rising to +1.0 at phase=0xFFFF.
   * phase is 0..0xFFFF unsigned; reinterpret as signed and it is already
   * -32768..+32767, which is exactly Q1.15.
   */
  return (SQ1_15)phase;
}

static inline SQ1_15 lfo_ramp(uint16_t phase)
{
  /* Falling ramp: invert the saw */
  return (SQ1_15)(~phase); /* ~phase = 0xFFFF - phase, then reinterpret */
}

static inline SQ1_15 lfo_square(uint16_t phase)
{
  return (phase < 0x8000u) ? (SQ1_15)0x7FFF : (SQ1_15)(-0x7FFF);
}

static uint32_t xorshift32(uint32_t *state)
{
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

static inline SQ1_15 lfo_sample_and_hold(struct lfo *lfo)
{
  static uint32_t rand_state = 2463534242u;

  /* Wrap detected: current phase wrapped below previous */
  if (lfo->phase < lfo->prev_phase)
  {
    /* Scale 32-bit random to signed Q1.15 by taking the top 16 bits */
    lfo->sh_value = (int16_t)(xorshift32(&rand_state) >> 16);
  }

  return lfo->sh_value;
}

void lfo_init(struct lfo *lfo)
{
  // TRACE_ASSERT(lfo);

  lfo->sh_value = 0;
  lfo_reset(lfo);
}

void lfo_reset(struct lfo *lfo)
{
  // TRACE_ASSERT(lfo);

  lfo->phase = 0u;
  lfo->prev_phase = 0u;
}

void lfo_render(struct lfo *lfo,
                SQ1_15 *tri, SQ1_15 *saw, SQ1_15 *ramp,
                SQ1_15 *square, SQ1_15 *sandh)
{
  // TRACE_ASSERT(lfo);
  // TRACE_ASSERT(tri);
  // TRACE_ASSERT(saw);
  // TRACE_ASSERT(ramp);
  // TRACE_ASSERT(square);
  // TRACE_ASSERT(sandh);

  *tri = lfo_triangle(lfo->phase);
  *saw = lfo_saw(lfo->phase);
  *ramp = lfo_ramp(lfo->phase);
  *square = lfo_square(lfo->phase);
  *sandh = lfo_sample_and_hold(lfo); /* must be called before phase update */

  lfo->prev_phase = lfo->phase;
  lfo->phase += lfo->inc;
}

void lfo_update(struct lfo *lfo, uint8_t rate, enum lfo_mode mode)
{
  // TRACE_ASSERT(lfo);

  lfo->rate = rate;
  lfo->mode = mode;
  lfo->inc = lfo_inc_lut[rate];
}

void lfo_note_on(struct lfo *lfo)
{
  // TRACE_ASSERT(lfo);

  if (lfo->mode == LFO_MODE_NOTE)
  {
    lfo_reset(lfo);
  }
}