#include "console.h"

#include "usb-cdc.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "debug.h"

/*
 * Input handling
 */

static unsigned char console_inbuf[1024];
static size_t console_inlen = 0;

static void console_send_prompt(void)
{
	
}

/*
 * Public API
 */

void cprintf(const char *fmt, ...)
{
	va_list ap;
	int len;
	char tx_buf[1024];
	va_start(ap, fmt);
	len = vsnprintf(tx_buf, sizeof(tx_buf), fmt, ap);
	va_end(ap);
	cdcacm_write_sync(tx_buf, len);
}

void console_poll(void)
{
	register char *sp __asm__("sp");
	while (1) {
		cdcacm_poll_usb();
		console_inlen = cdcacm_read_sync(console_inbuf, sizeof(console_inbuf));

		cprintf("starting malloc test\r\n");
		gpio_set(DBGO, DBG_G);
		for (int i = 1; i < 20000; i *= 2) {
			cprintf("at stack position %08x\r\n", sp);
			cprintf("trying to malloc %i bytes, brk=%08x\r\n", i, _sbrk(0));
			gpio_clear(DBGO, DBG_B);
			void *p = malloc(i);
			gpio_clear(DBGO, DBG_R);
			if (! p) {
				cprintf("malloc(%i) failed: %i (%s)\r\n", i, errno, strerror(errno));
				break;
			}
			cprintf("malloc(%i) successful, result=%08x, brk=%08x\r\n", i, (intptr_t)p, _sbrk(0));
			gpio_set(DBGO, DBG_B);
			free(p);
			gpio_set(DBGO, DBG_R);
		}

	}
	// cdcacm_poll_usb();
}
