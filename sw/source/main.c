/*
 *  (c) Jason Wilden, 2026
 */

#include "drv.h"
#include "controller.h"

#define IRQ_TIMER (1u)
#define IRQ_AUDIO (1u << 3)
#define IRQ_MIDI (1u << 4)

#define TIMER_COUNT (6000000UL)

static volatile uint8_t irq_count = 0;
static struct controller controller;
static volatile bool blink = false;

static inline uint32_t reload_timer(uint32_t val);

/*
 * Firmware entry point
 */
int main(void)
{
  bool led = true;

  trace_init(TRACE);
  trace_print(TRACE, "=== TangSynth9 Template ===\n");
  controller_init(&controller);

  reload_timer(TIMER_COUNT);

  while (1)
  {
    irq_count = controller_execute(&controller, irq_count);
  }

  /*NOTREACHED*/
  return 0;
}

/*
* Interrupt service routine.  Note that the mask can have multiple
* IRQ bits so there is an implied priority in handling them 
* from the order of the IRQ checks in the function.
*/
void handle_irq(uint32_t mask)
{
  if (mask & IRQ_TIMER)
  {
    blink = !blink;
    blink ? gpo_set_pin(GPO1,GPO_BSR_3) : gpo_clear_pin(GPO1,GPO_BSR_3);
    reload_timer(TIMER_COUNT);
  }

  if (mask & IRQ_AUDIO)
  {        
    irq_count++;    
  }

  if (mask & IRQ_MIDI)
  {    
    if (midi_rx_ready(MIDI))
    {
      midi_buffer_write(midi_read_byte(MIDI));
    }        
  }  
}

// Reload the countdown timer (IRQ 0)
static inline uint32_t reload_timer(uint32_t val)
{
  uint32_t retval;
  __asm__ volatile(
      ".insn r 0b0001011, 0b110, 0b0000101, %0, %1, x0\n"
      : "=r"(retval)
      : "r"(val));
  return retval;
}
