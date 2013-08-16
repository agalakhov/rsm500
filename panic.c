#include "panic.h"

#include "common.h"

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/gpio.h>

enum panic_domain {
	PANIC_USER = 1,
	PANIC_ARM = 2,
	PANIC_MEM = 3,
};

enum panic_arm {
	PANIC_HARD_FAULT = 1,
	PANIC_MEM_MANAGE = 2,
	PANIC_BUS_FAULT = 3,
	PANIC_USAGE_FAULT = 4,
};

enum panic_mem {
	PANIC_STACK_SMASH = 1,
};

enum panic_delay {
	PANIC_SHORT = 1000000,
	PANIC_LONG = 3 * PANIC_SHORT,
};

/*
 *  Panic function internal implementation
 */

static inline void panic_delay(enum panic_delay delay)
{
	unsigned i;
	for (i = 0; i < delay; ++i)
		barrier();
}

static inline void panic_led_on(void)
{
	gpio_set(GPIOB, GPIO1);
}

static inline void panic_led_off(void)
{
	gpio_clear(GPIOB, GPIO1);
}

static void NORETURN do_panic(enum panic_domain domain,
	unsigned num_blinks)
{
	cm_disable_interrupts();
	while (1) {
		unsigned i;
		for (i = 0; i < (unsigned) domain; ++i) {
			panic_led_on();
			panic_delay(PANIC_LONG);
			panic_led_off();
			panic_delay(PANIC_SHORT);
		}
		for (i = 0; i < num_blinks; ++i) {
			panic_led_on();
			panic_delay(PANIC_SHORT);
			panic_led_off();
			panic_delay(PANIC_SHORT);
		}
		panic_delay(PANIC_LONG);
	}
}


/*
 *  Public API
 */

void NORETURN panic(unsigned num_blinks)
{
	do_panic(PANIC_USER, num_blinks);
}


/*
 *  ARM signal handlers
 */

void hard_fault_handler(void)
{
	do_panic(PANIC_ARM, PANIC_HARD_FAULT);
}

void mem_manage_handler(void)
{
	do_panic(PANIC_ARM, PANIC_MEM_MANAGE);
}

void bus_fault_handler(void)
{
	do_panic(PANIC_ARM, PANIC_BUS_FAULT);
}

void usage_fault_handler(void)
{
	do_panic(PANIC_ARM, PANIC_USAGE_FAULT);
}

/*
 *  Stack smashing protector
 */

void NORETURN  __stack_chk_fail()
{
	do_panic(PANIC_MEM, PANIC_STACK_SMASH);
}

/* Stack check guard canary */
void *__stack_chk_guard = (void *) 0xFF0F0A07;
