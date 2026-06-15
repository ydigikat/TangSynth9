#------------------------------------------------------------------------------
# Jason Wilden 2025
#------------------------------------------------------------------------------
# picorv32 startup with IRQ support.
#
# Memory layout:
#   0x00000000: Reset vector
#   0x00000010: Interrupt vector
#
# IRQ mechanism:  On interrupt CPU:
#   - Saves PC -> q0
#   - Saves IRQ mask -> q1
#   - Disables further IRQs
#   - Jumps to 0x00000010
# The software must save/restore all GP registers.
# 
#------------------------------------------------------------------------------

# Custom ops for *_insn macros.
#include "custom_ops.S"             

# =============================================================================
# Reset Vector (0x00000000)
# =============================================================================
.section .text.start
.global _start
.type _start, @function

_start:
  # Max 16 bytes for reset vector
  picorv32_maskirq_insn(zero, zero)   # Unmask all interrupts
	j _reset                            # Jump to reset code
    
.size _start, .-_start        

# =============================================================================
# IRQ vector (0x00000010)
# =============================================================================
.section .text.irq
.global _irq_handler
.type _irq_handler, @function
.balign 16                                # Ensure aligned.

_irq_handler:

  # ---------------------------------------------------------------------------
  # On entry:
  #   q0 - has PC saved.
  #   q1 - IRQ bitmask (which lines fired)
  #   x registers not saved.
  #   Further IRQs disabled
  #   sp valid.
  # ---------------------------------------------------------------------------

  # Allocate stack frame for preserved registers (16x4 bytes = 64)
  addi sp, sp, -64

  # Save caller-saved registers
  sw ra, 0(sp)        # return address (clobbered by jal to irq())
  sw t0, 4(sp)        # temporaries
  sw t1, 8(sp)
  sw t2, 12(sp)
  sw a0, 16(sp)       # Arguments
  sw a1, 20(sp)
  sw a2, 24(sp)
  sw a3, 28(sp)
  sw a4, 32(sp)
  sw a5, 36(sp)
  sw a6, 40(sp)
  sw a7, 44(sp)
  sw t3, 48(sp)       # Temporaries
  sw t4, 52(sp)
  sw t5, 56(sp)
  sw t6, 60(sp)

  # retrieve the bitmask into a0 (arg 1)
  picorv32_getq_insn(a0,q1)

  # Call handler: void irq(uint32_t mask)
  call handle_irq

  # Restore caller-saved registers
  lw ra, 0(sp)        # return address (clobbered by jal to irq())
  lw t0, 4(sp)        # temporaries
  lw t1, 8(sp)
  lw t2, 12(sp)
  lw a0, 16(sp)       # Arguments
  lw a1, 20(sp)
  lw a2, 24(sp)
  lw a3, 28(sp)
  lw a4, 32(sp)
  lw a5, 36(sp)
  lw a6, 40(sp)
  lw a7, 44(sp)
  lw t3, 48(sp)       # Temporaries
  lw t4, 52(sp)
  lw t5, 56(sp)
  lw t6, 60(sp)  

  addi sp, sp, 64     # deallocate stack frame
  
  # return from interrupt
  picorv32_retirq_insn()

  .size _irq_handler, .-_irq_handler

# =============================================================================
# Reset Handler - C startup
# =============================================================================

.section .text
.global _reset
.type _reset, @function

_reset:
    # Set up stack pointer
    lui     sp, %hi(__stack_top)
    addi    sp, sp, %lo(__stack_top)

    # Clear BSS
    la      t0, __bss_start
    la      t1, __bss_end
bss_clear:
    beq     t0, t1, bss_done
    sw      zero, 0(t0)
    addi    t0,  t0, 4
    j       bss_clear
bss_done:
    call    main

loop_forever:
    j       loop_forever

.size _reset, .-_reset
