/*
*  (c) Jason Wilden, 2026
*/
#ifndef __FIXED_H__
#define __FIXED_H__

#include <stdint.h>

typedef uint16_t Q1_15;
typedef uint32_t Q1_24;

#define Q15_ONE   ((Q1_15)0x7FFF)  /* ~0.99997, max representable Q1.15 */
#define Q15_SHIFT (15)


/**
 * \brief Q1.15 x Q1.15 -> Q1.15 multiply, signed, with rounding shift.
 *
 * 16x16 multiply widens to 32 bits (fits a single PicoRV32 MUL instruction),
 * then shift right 15 to bring back to Q1.15.
 */
inline int16_t q15_mul(int16_t a, int16_t b)
{
  int32_t result = (int32_t)a * (int32_t)b;
  return (int16_t)(result >> Q15_SHIFT);
}

#endif /* __FIXED_H__*/