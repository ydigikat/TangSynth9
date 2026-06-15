/*
 *  Estella : (c) Jason Wilden, 2026
 */

#include "drv.h" 
#include "midi.h"

static volatile uint32_t aud_irq_count;

int main(void)
{
  uint8_t led = 0;
  
  trace_init(TRACE);
  trace_print(TRACE, "Tracing API initialised.\n");
  while (1)
  {  
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
    
  }

  if (mask & IRQ_AUDIO)
  {
    aud_irq_count++;
  }
}
