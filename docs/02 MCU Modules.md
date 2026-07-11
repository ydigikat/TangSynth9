# MCU Modules
A small set of memory mapped IO (MMIO) modules are implemented for use by the MCU, this is not a comprehensive set of peripherals but only those required specifically for the synthesiser.

These are mapped into the address space starting at 0x8000000 and each has a 0x40 sized register address space.

| Module | Purpose |
| ------ | ------- |
| SRAM   | CPU SRAM (BRAM) |
| VRAM   | Shared Voice RAM |
| VRCR   | VRAM control register |
| TRACE  | Serial-tx module (used for debug/trace) |
| MIDI   | Serial-rx module (used for MIDI in) |
| GPO    | On-board LEDs and misc IO|

The modules use the native memory interface provided by the picorv32, this is not described in the module documentation as it is the same for all and well covered by the picorv32 documentation. 


## MIDI Module

The MIDI module is only responsible for receiving data bytes and signalling this to the MCU.  It does not parse MIDI data.  It is a simple wrapper around the ```serial_rx``` RTL module.

### Registers

| Register | Bits| Field |
| -------- | --- | ------- |
| CR       | [10:0] | Baud rate divisor|
| SR       | [0] | RX valid|
|          | [1] | Framing error |
| RD       | [7:0] | Data byte |

### Interface

| Signal | Purpose |
| ------ | ------- |
| rx_i | Serial data in|
| irq_o | MIDI byte received interrupt |

## GPO Module

General purpose output module.  The state of a pin can be controlled using software however the pin assigment cannot, this is defined by the RTL.

Pins 0-5 are wired to the user LEDs on the Tang Nano 9K development board. The other pins are left unconnected in the template. 

Bits 0-15 are used to set a pin, corresponding bits 16-31 are used to reset a pin.  Reading the register will return zeros (no state).

### Registers


| Register | Bits| Field |
| -------- | --- | ------- |
| BSR | [15:0] | Set pins 0-15 on |
|     | [31:15] | Set pins 0-15 off |


### Interface

| Signal | Purpose |
| ------ | ------- |
| gpo_o[15:0] | GPO Pins |

