#include "rsm500-usb.h"

#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include "console.h"

#include "debug.h"


void sys_tick_handler(void)
{
	gpio_toggle(GPIOB, GPIO1);
}

int main(void)
{
	/* Low level initialization. */
	extern uint32_t vector_table __asm__("vector_table"); /* Defined by the linker */
	SCB_VTOR = (uint32_t) &vector_table;
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Enable clock for Port B used by the LED and USB pull-up transistor */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO8);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO9);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO10);

	gpio_set(DBGO, DBG_R);
	gpio_clear(DBGO, DBG_G);
	gpio_set(DBGO, DBG_B);

	/* Setup GPIOB Pin 1 for the LED */
	gpio_set(GPIOB, GPIO1);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);

	rsm500_usb_init();

	systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB);
	systick_set_reload(8999999);
	systick_interrupt_enable();
	systick_counter_enable();

	//cprintf("RSM-500 spectrometer ready\n");
	while (1) {
//		cdcacm_poll_usb();
		console_poll();

	}

	return 0;
}
