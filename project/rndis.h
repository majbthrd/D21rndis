/*
  This is very loosely derived from lrndis/usbd_rndis_core.c from https://github.com/fetisov/lrndis
*/

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _RNDIS_H_
#define _RNDIS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rndis_protocol.h"

#define RNDIS_MTU        1500                           /* MTU value */
#define RNDIS_LINK_SPEED 12000000                       /* Link baudrate (12Mbit/s for USB-FS) */
#define RNDIS_VENDOR     "acme"                         /* NIC vendor name */
#define RNDIS_HWADDR     0x20,0x89,0x84,0x6A,0x96,0xAB  /* MAC-address to set to host interface */

#define ETH_HEADER_SIZE             14
#define ETH_MAX_PACKET_SIZE         ETH_HEADER_SIZE + RNDIS_MTU
#define RNDIS_BUFFER_SIZE           (ETH_MAX_PACKET_SIZE + sizeof(rndis_data_packet_t))

void rndis_recv_callback(uint8_t *data, int size);
void rndis_class_set_handler(uint8_t *data, int size);
void rndis_send(struct pbuf *p);

#endif
