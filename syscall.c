#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include <libopencm3/stm32/gpio.h>

enum {
	STACK_RESERVE = 128,
};

/*
 *  Memory allocator
 */

caddr_t _sbrk(int incr)
{
	extern char end __asm__("end"); /* Defined by the linker */
	register char *sp __asm__("sp");
	static caddr_t heap_end = &end;

	caddr_t prev_heap_end;
	prev_heap_end = __sync_fetch_and_add(&heap_end, incr);
	if (prev_heap_end + incr > sp - STACK_RESERVE) {
		(void) __sync_sub_and_fetch(&heap_end, incr);
		errno = ENOMEM;
		return (caddr_t) -1;
	}
	return prev_heap_end;
}
