#ifndef XRT_H
#define XRT_H

#define XRT_USB
#define XRT_NETWORK
#define XRT_PS2MOUSE
#define XRT_MONITORING

#include <rttarget.h>
#include <rtk32.h>
#include <rtipapi.h>
#include <finetime.h>
#include <stdio.h>
#include <stdlib.h>
#include <socket.h>
#include <clock.h>
#include <rtusb.h>


void xrt_init();

#endif