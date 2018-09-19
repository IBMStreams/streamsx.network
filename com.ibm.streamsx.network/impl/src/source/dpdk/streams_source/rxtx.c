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
#include <time.h>

#define DPDK_INST_ENABLE

#define DPDK_INST_BUCKET_SIZE 60000000000UL
#ifdef DPDK_INST_ENABLE
#define DPDK_INST_DELTA_TS_START clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_before)
#define DPDK_INST_DELTA_TS_END   clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_after); \
    deltaT = (ts_after.tv_sec - ts_before.tv_sec)*1000000000UL + ts_after.tv_nsec - ts_before.tv_nsec

#else
#define DPDK_INST_DELTA_TS_START
#define DPDK_INST_DELTA_TS_END
#endif

#define PREFETCH_OFFSET 1
void receive_loop(struct lcore_conf *conf) {
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    unsigned i, portid;
    int j, num_rx, count;

#ifdef DPDK_INST_ENABLE
    // The instrumentation variables
    uint64_t bucketStartTime = 0;
    uint64_t bucketPriorWastedTime = 0;
    uint64_t bucketBurstFoundCount = 0;
    uint64_t bucketNoneFoundCount = 0;
    uint64_t bucketPacketCountTotal = 0;
    uint64_t bucketPacketCountSquareTotal = 0;
    uint64_t bucketPacketCountMin = UINT64_MAX;
    uint64_t bucketPacketCountMax = 0;
    uint64_t bucketBurstFoundDurationTotal = 0;
    uint64_t bucketBurstFoundDurationSquareTotal = 0;
    uint64_t bucketBurstFoundDurationMin = UINT64_MAX;
    uint64_t bucketBurstFoundDurationMax = 0;
    uint64_t bucketNoneFoundDurationTotal = 0;
    uint64_t bucketNoneFoundDurationSquareTotal = 0;
    uint64_t bucketNoneFoundDurationMin = UINT64_MAX;
    uint64_t bucketNoneFoundDurationMax = 0;
    uint64_t bucketCallbackDurationTotal = 0;
    uint64_t bucketCallbackDurationSquareTotal = 0;
    uint64_t bucketCallbackDurationMin = UINT64_MAX;
    uint64_t bucketCallbackDurationMax = 0;
    uint64_t bucketPacketFreeDurationTotal = 0;
    uint64_t bucketPacketFreeDurationSquareTotal = 0;
    uint64_t bucketPacketFreeDurationMin = UINT64_MAX;
    uint64_t bucketPacketFreeDurationMax = 0;
#endif


    RTE_LOG(INFO, STREAMS_SOURCE, "Starting receive loop on lcore %u\n",
	    rte_lcore_id());
    RTE_LOG(INFO, STREAMS_SOURCE, "conf = 0x%lx, num_rx_queue = %d\n",
	    conf, conf->num_rx_queue);

    for (i = 0; i < conf->num_rx_queue; i++) {
        RTE_LOG(INFO, STREAMS_SOURCE, "port_id = %d, queue_id = %d\n", 
                conf->rx_queue_list[i].port_id,
                conf->rx_queue_list[i].queue_id);
    }

#ifdef DPDK_INST_ENABLE
    // Some other timestamp holders
    struct timespec ts_before;
    struct timespec ts_after;
    uint64_t deltaT;

    // save this off for easier display
    unsigned lcore_id = rte_lcore_id();
    int port_id = conf->rx_queue_list[0].port_id;
    int queue_id = conf->rx_queue_list[0].queue_id;

    // Get the timestamp we started the first bucket
    struct timespec ts_loop;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_loop);
    bucketStartTime = ts_loop.tv_nsec + ts_loop.tv_sec * 1000000000UL;
#endif

    while(1) {
	// Read rx packets from the queue.
	for (i = 0; i < conf->num_rx_queue; i++) {
	    portid = conf->rx_queue_list[i].port_id;
	    streams_packet_cb_t packetCallback = conf->rx_queue_list[i].packetCallbackFunction;

            DPDK_INST_DELTA_TS_START;
	    num_rx = rte_eth_rx_burst((uint32_t) portid, conf->rx_queue_list[i].queue_id,
		                      pkts_burst, MAX_PKT_BURST);
            DPDK_INST_DELTA_TS_END;

	    if (num_rx) {
#ifdef DPDK_INST_ENABLE
                ++bucketBurstFoundCount;

                bucketPacketCountTotal += num_rx;
                bucketPacketCountSquareTotal += num_rx * num_rx;
                if(num_rx < bucketPacketCountMin) {
                    bucketPacketCountMin = num_rx;
                }
                if(num_rx > bucketPacketCountMax) {
                    bucketPacketCountMax = num_rx;
                }

                bucketBurstFoundDurationTotal += deltaT;
                bucketBurstFoundDurationSquareTotal += deltaT*deltaT;
                if(deltaT < bucketBurstFoundDurationMin) {
                    bucketBurstFoundDurationMin = deltaT;
                }
                if(deltaT > bucketBurstFoundDurationMax) {
                    bucketBurstFoundDurationMax = deltaT;
                }
#endif

		for (j = 0; j < PREFETCH_OFFSET && j < num_rx; j++) {
		    rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));
		}

		for (j = 0; j < (num_rx - PREFETCH_OFFSET); j++) {
		    rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j
				+ PREFETCH_OFFSET], void *));

                    if(packetCallback) {
                        DPDK_INST_DELTA_TS_START;
		        packetCallback(conf->rx_queue_list[i].userData,
		                       rte_pktmbuf_mtod(pkts_burst[j], char *),
			    	       pkts_burst[j]->data_len, rte_rdtsc());
                        DPDK_INST_DELTA_TS_END;
#ifdef DPDK_INST_ENABLE
                        bucketCallbackDurationTotal += deltaT;
                        bucketCallbackDurationSquareTotal += deltaT*deltaT;
                        if(deltaT < bucketCallbackDurationMin) {
                            bucketCallbackDurationMin = deltaT;
                        }
                        if(deltaT > bucketCallbackDurationMax) {
                            bucketCallbackDurationMax = deltaT;
                        }
#endif
                    }
                    DPDK_INST_DELTA_TS_START;
		    rte_pktmbuf_free(pkts_burst[j]);
                    DPDK_INST_DELTA_TS_END;
#ifdef DPDK_INST_ENABLE
                    bucketPacketFreeDurationTotal += deltaT;
                    bucketPacketFreeDurationSquareTotal += deltaT*deltaT;
                    if(deltaT < bucketPacketFreeDurationMin) {
                        bucketPacketFreeDurationMin = deltaT;
                    }
                    if(deltaT > bucketPacketFreeDurationMax) {
                        bucketPacketFreeDurationMax = deltaT;
                    }
#endif

		    count++;
		}

		for (; j < num_rx; j++) {
                    if(packetCallback) {
                        DPDK_INST_DELTA_TS_START;
		        packetCallback(conf->rx_queue_list[i].userData,
		                       rte_pktmbuf_mtod(pkts_burst[j], char *),
			    	       pkts_burst[j]->data_len, rte_rdtsc());
                        DPDK_INST_DELTA_TS_END;
#ifdef DPDK_INST_ENABLE
                        bucketCallbackDurationTotal += deltaT;
                        bucketCallbackDurationSquareTotal += deltaT*deltaT;
                        if(deltaT < bucketCallbackDurationMin) {
                            bucketCallbackDurationMin = deltaT;
                        }
                        if(deltaT > bucketCallbackDurationMax) {
                            bucketCallbackDurationMax = deltaT;
                        }
#endif
                    }
                    DPDK_INST_DELTA_TS_START;
		    rte_pktmbuf_free(pkts_burst[j]);
                    DPDK_INST_DELTA_TS_END;
#ifdef DPDK_INST_ENABLE
                    bucketPacketFreeDurationTotal += deltaT;
                    bucketPacketFreeDurationSquareTotal += deltaT*deltaT;
                    if(deltaT < bucketPacketFreeDurationMin) {
                        bucketPacketFreeDurationMin = deltaT;
                    }
                    if(deltaT > bucketPacketFreeDurationMax) {
                        bucketPacketFreeDurationMax = deltaT;
                    }
#endif
		    count++;
		}
	    } else {
#ifdef DPDK_INST_ENABLE
                ++bucketNoneFoundCount;
                bucketNoneFoundDurationTotal += deltaT;
                bucketNoneFoundDurationSquareTotal += deltaT*deltaT;
                if(deltaT < bucketNoneFoundDurationMin) {
                    bucketNoneFoundDurationMin = deltaT;
                }
                if(deltaT > bucketNoneFoundDurationMax) {
                    bucketNoneFoundDurationMax = deltaT;
                }
#endif
            }
	}

#ifdef DPDK_INST_ENABLE
        // Check to see if we should dump and reset the buckets
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_loop);
        uint64_t tempCurrentTime = ts_loop.tv_nsec + ts_loop.tv_sec * 1000000000UL;

        if(tempCurrentTime - bucketStartTime >= DPDK_INST_BUCKET_SIZE) {
            // Record how much extra time we're wasting here
            DPDK_INST_DELTA_TS_START;

            // Use this time to display only.
            struct timespec ts_real;
            clock_gettime(CLOCK_REALTIME, &ts_real);

            // Display the data for this bucket/thread to stdout for now
            printf("RXTX: receive_loop: METRICS %u %d %d %lu.%09lu %lu %lu : %lu %lu %lu %lu %lu : %lu %lu %lu %lu %lu : %lu %lu %lu %lu : %lu %lu %lu %lu : %lu %lu %lu %lu\n",
                   lcore_id, port_id, queue_id, ts_real.tv_sec, ts_real.tv_nsec, bucketPriorWastedTime, (tempCurrentTime - bucketStartTime),
                   bucketNoneFoundCount, bucketNoneFoundDurationTotal, bucketNoneFoundDurationSquareTotal, bucketNoneFoundDurationMin, bucketNoneFoundDurationMax,
                   bucketBurstFoundCount, bucketBurstFoundDurationTotal, bucketBurstFoundDurationSquareTotal, bucketBurstFoundDurationMin, bucketBurstFoundDurationMax,
                   bucketPacketCountTotal, bucketPacketCountSquareTotal, bucketPacketCountMin, bucketPacketCountMax,
                   bucketCallbackDurationTotal, bucketCallbackDurationSquareTotal, bucketCallbackDurationMin, bucketCallbackDurationMax,
                   bucketPacketFreeDurationTotal, bucketPacketFreeDurationSquareTotal, bucketPacketFreeDurationMin, bucketPacketFreeDurationMax);

            // Reset the counters for next time around, other than the starttime and wasted time, which will be updated after this and kept.
            // bucketStartTime = 0;
            // bucketPriorWastedTime = 0;
            bucketBurstFoundCount = 0;
            bucketNoneFoundCount = 0;
            bucketPacketCountTotal = 0;
            bucketPacketCountSquareTotal = 0;
            bucketPacketCountMin = UINT64_MAX;
            bucketPacketCountMax = 0;
            bucketBurstFoundDurationTotal = 0;
            bucketBurstFoundDurationSquareTotal = 0;
            bucketBurstFoundDurationMin = UINT64_MAX;
            bucketBurstFoundDurationMax = 0;
            bucketNoneFoundDurationTotal = 0;
            bucketNoneFoundDurationSquareTotal = 0;
            bucketNoneFoundDurationMin = UINT64_MAX;
            bucketNoneFoundDurationMax = 0;
            bucketCallbackDurationTotal = 0;
            bucketCallbackDurationSquareTotal = 0;
            bucketCallbackDurationMin = UINT64_MAX;
            bucketCallbackDurationMax = 0;
            bucketPacketFreeDurationTotal = 0;
            bucketPacketFreeDurationSquareTotal = 0;
            bucketPacketFreeDurationMin = UINT64_MAX;
            bucketPacketFreeDurationMax = 0;

            // Reset the start time for the next bucket
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_loop);
            bucketStartTime = ts_loop.tv_nsec + ts_loop.tv_sec * 1000000000UL;

            DPDK_INST_DELTA_TS_END;
            bucketPriorWastedTime = deltaT;
        }
#endif
    }
}

