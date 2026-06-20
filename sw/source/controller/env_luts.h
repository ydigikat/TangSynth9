/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __ENV_LUTS_H__
#define __ENV_LUTS_H__

#include <stdint.h>
#include "types.h"

extern const Q1_15  env_attack_coeff_lut[128];
extern const Q1_15  env_attack_overshoot_lut[128];      /* always >= 0, safe as unsigned */
extern const Q1_15  env_decay_coeff_lut[128];           /* als the release_coeff */
extern const int16_t env_decay_overshoot_base_lut[128]; /* SIGNED - always <= 0 */
extern const int16_t env_release_overshoot_lut[128];    /* SIGNED - always <= 0 */


#endif /* __ENV_LUTS_H__ */