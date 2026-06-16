/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "midi.h"

enum param_mapping_type
{
  PARAM_UNMAPPED,
  PARAM_VOICE
};

/* Voice parameters */
enum param_id
{                   
  MASTER_LEVEL,     
  //TODO Rest here
  VOICE_PARAM_COUNT
};



#endif /* __PARAMS_H__ */