/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __ENV_LUTS_H__
#define __ENV_LUTS_H__

#include <stdint.h>
#include "types.h"

#define ENV_DECAY_OVERSHOOT_CUTOFF  25
#define ENV_RELEASE_OVERSHOOT_CUTOFF 25

extern const Q1_15  env_attack_coeff_lut[128];
extern const Q1_15  env_attack_overshoot_lut[128];
extern const Q1_15  env_decay_coeff_lut[128];                                  
extern const int16_t env_decay_overshoot_base_lut[ENV_DECAY_OVERSHOOT_CUTOFF]; 
extern const int16_t env_release_overshoot_lut[ENV_RELEASE_OVERSHOOT_CUTOFF];  

/*
 * Bounds-checked lookup into a truncated ADSR TCO LUT.
 */
static inline int16_t env_overshoot_lookup(const int16_t *table, uint8_t cutoff, uint8_t midi_val)
{
  return (midi_val < cutoff) ? table[midi_val] : 0;
}

#endif /* __ENV_LUTS_H__ */