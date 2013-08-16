/* Linker script for Leaf Maple (STM32F103CBT6, 128K flash, 20K RAM,
 * minus 20k flash and 3k RAM for the DFU bootloader). */

/* Define memory regions. */
MEMORY
{
	rom (rx) : ORIGIN = 0x08005000, LENGTH = 108K
	ram (rwx) : ORIGIN = 0x20000c00, LENGTH = 17K
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f1.ld

