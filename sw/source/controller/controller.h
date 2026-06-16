/*
*  (c) Jason Wilden, 2026
*/
#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <stdint.h>
#include <stddef.h>

#include "types.h"
#include "midi.h"
#include "voice.h"
#include "params.h"

#define MAX_VOICES (8)

struct controller
{
  uint8_t midi_channel;

  struct voice voice[MAX_VOICES];
  uint8_t params[VOICE_PARAM_COUNT];
};

/* API */
void controller_init(struct controller *controller);
void controller_handle_midi(struct controller *controller, struct midi_msg *msg);
void controller_calculate(struct controller *controller);

#endif /* __CONTROLLER_H__*/