/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 *********************************************************************/

#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include "rxtx.h"
#include "config.h"

#define PREFETCH_OFFSET 1
void receive_loop(struct lcore_conf *conf) {
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    unsigned i, portid;
    int j, num_rx, count;

    RTE_LOG(INFO, STREAMS_SOURCE, "Starting receive loop on lcore %u\n",
	    rte_lcore_id());
    RTE_LOG(INFO, STREAMS_SOURCE, "conf = 0x%lx, num_rx_queue = %d\n",
	    conf, conf->num_rx_queue);

    while(1) {
	// Read rx packets from the queue.
	for (i = 0; i < conf->num_rx_queue; i++) {
	    portid = conf->rx_queue_list[i].port_id;
	    streams_packet_cb_t packetCallback = conf->rx_queue_list[i].packetCallbackFunction;
	    num_rx = rte_eth_rx_burst((uint32_t) portid, conf->rx_queue_list[i].queue_id,
		                      pkts_burst, MAX_PKT_BURST);
	    if (num_rx) {
		for (j = 0; j < PREFETCH_OFFSET && j < num_rx; j++) {
		    rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));
		}

		for (j = 0; j < (num_rx - PREFETCH_OFFSET); j++) {
		    rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j
				+ PREFETCH_OFFSET], void *));

                    if(packetCallback) {
		        packetCallback(conf->rx_queue_list[i].userData,
		                       rte_pktmbuf_mtod(pkts_burst[j], char *),
			    	       pkts_burst[j]->data_len, rte_rdtsc());
                    }
		    rte_pktmbuf_free(pkts_burst[j]);
		    count++;
		}

		for (; j < num_rx; j++) {
                    if(packetCallback) {
		        packetCallback(conf->rx_queue_list[i].userData,
		                       rte_pktmbuf_mtod(pkts_burst[j], char *),
			    	       pkts_burst[j]->data_len, rte_rdtsc());
                    }
		    rte_pktmbuf_free(pkts_burst[j]);
		    count++;
		}
	    }
	}
    }
}

