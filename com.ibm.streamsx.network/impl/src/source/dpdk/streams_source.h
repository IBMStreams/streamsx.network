/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 *********************************************************************/


#ifndef _STREAMS_SOURCE_H_
#define _STREAMS_SOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif
    struct port_stats {
	uint64_t received;
	uint64_t dropped;
	uint64_t bytes;
    };

    typedef void (*streams_packet_cb_t)(void * user,void *data, 
                  unsigned int len,
	          uint64_t timestamp);

    int streams_master_init(const char* coreMask, const char* portMask, int numQueues, 
                            int lcore, streams_packet_cb_t callback, void *user);

    int streams_slave_init(int lcore, int nicPort, int nicQueue, int promiscuous,
                           streams_packet_cb_t callback, void *user);

    uint64_t streams_source_get_tsc_hz(void);

    int streams_source_start(int isMaster);

    int streams_port_stats(int, struct port_stats *);

#ifdef __cplusplus
}
#endif

#endif /* _MAIN_H_ */
