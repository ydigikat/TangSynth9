/*
 *  Estella : (c) Jason Wilden, 2026
 */

#include "drv.h" 
#include "midi.h"

static volatile uint32_t aud_irq_count;
static struct midi_instance midi_in;


int main(void)
{
  uint8_t led = 0;
  
  trace_init(TRACE);
  trace_print(TRACE, "Tracing API initialised.\n");


  while (1)
  {  
    /* Process pending MIDI events from buffer */
    uint8_t byte;
    while(midi_buffer_read(&byte))
    {
      struct midi_msg *msg = midi_parse(&midi_in, byte);
      if (msg != NULL)
      {
        trace_printf(TRACE,"MIDI: %x, %x, %x\n", msg->data[0], msg->data[1], msg->data[2]);
      }
    }

    /* Heartbeat to show we're still alive and getting audio interrupts */
    if (aud_irq_count == 46880)
    {
      led = !led;
      led ? gpo_set_pin(GPO1, GPO_BSR_2) : gpo_clear_pin(GPO1, GPO_BSR_2);
      aud_irq_count = 0;
    }
  }

  /*NOTREACHED*/
  return 0;
}

#define IRQ_AUDIO (1u << 3)
#define IRQ_MIDI (1u << 4)

void handle_irq(uint32_t mask)
{  
  if (mask & IRQ_MIDI)
  {    
    /*
    * Incoming MIDI byte, read from register and copy to midi buffer.
    */
    uint8_t byte = MIDI_STATUS_ACTIVE_SENSE; /* Temporary - MIDI NOP */

    midi_buffer_write(byte);
  }

  if (mask & IRQ_AUDIO)
  {
    /*
    * Audio interrupt, update VRAM with parameter changes (if complete) and signal PCR for
    * audio pipeline to read.
    */

    aud_irq_count++;    /* Temporary - count interrupts */
  }
}
