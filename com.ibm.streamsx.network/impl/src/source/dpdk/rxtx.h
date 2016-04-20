/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 *********************************************************************/

#ifndef _RXTX_H__
#define _RXTX_H__

#include "config.h"
#include "streams_source.h"

void receive_loop(struct lcore_conf *conf);
extern streams_packet_cb_t packet_cb;
extern void *user_data;

#endif

