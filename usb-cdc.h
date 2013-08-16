#pragma once

#include <stdint.h>
#include <stddef.h>

#include <libopencm3/usb/usbd.h>

void cdcacm_set_dev(usbd_device *dev);

extern const struct usb_interface_descriptor cdcacm_comm_iface;
extern const struct usb_interface_descriptor cdcacm_data_iface;

typedef void (*io_cb_t) (void *buf, size_t size, intptr_t parm);

int cdcacm_read(void *buf, size_t size, io_cb_t cb, intptr_t p);
int cdcacm_write(const void *buf, size_t size, io_cb_t cb, intptr_t p);

int cdcacm_read_sync(void *buf, size_t size);
int cdcacm_write_sync(const void *buf, size_t size);

void cdcacm_poll_usb(void);
