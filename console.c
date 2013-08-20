#include "console.h"

#include "usb-cdc.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "debug.h"

#include <unistd.h>

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
	console_inlen = cdcacm_read_sync(console_inbuf, sizeof(console_inbuf));
	//cdcacm_poll_usb();
	cdcacm_write_sync(console_inbuf, console_inlen);
}
