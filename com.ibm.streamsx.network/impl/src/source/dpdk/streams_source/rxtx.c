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

#define DPDK_INST_BUCKET_SIZE 60UL
#define DPDK_INST_BUCKET_COUNT 10
#define DPDK_INST_SCRATCH_SIZE 4096
#ifdef DPDK_INST_ENABLE
#define DPDK_INST_TS(var) var = rte_rdtsc()
#define DPDK_INST_UPDATE_METRIC(m, d) addToMetric(&buckets[bucketIndex].m, (d))
#else
#define DPDK_INST_TS(var)
#define DPDK_INST_UPDATE_METRIC(m, d)
#endif

struct Metric {
    uint64_t count;
    uint64_t total;
    uint64_t squareTotal;
    uint64_t min;
    uint64_t max;
};

struct Bucket {
    uint64_t duration;
    uint64_t priorWastedTime;
    struct Metric packetCount;
    struct Metric burstFoundDuration;
    struct Metric noneFoundDuration;
    struct Metric callbackDuration;
    struct Metric packetFreeDuration;
};

inline __attribute__((__always_inline__))
void addToMetric(struct Metric *m, uint64_t datum) {
    ++m->count;
    m->total += datum;
    m->squareTotal += datum * datum;
    if(datum < m->min) {
        m->min = datum;
    }
    if(datum > m->max) {
        m->max = datum;
    }
}



#define PREFETCH_OFFSET 1
void receive_loop(struct lcore_conf *conf) {
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    unsigned i, portid;
    int j, num_rx, count;

#ifdef DPDK_INST_ENABLE
    // Some timestamp holders
    uint64_t ts_A, ts_B, ts_C, ts_D, ts_E, ts_F, ts_G;

    // save these off for easier display and computation later
    const uint64_t bucketTargetSize = DPDK_INST_BUCKET_SIZE * rte_get_tsc_hz();
    const unsigned lcore_id = rte_lcore_id();
    const int port_id = conf->rx_queue_list[0].port_id;
    const int queue_id = conf->rx_queue_list[0].queue_id;

    // The instrumentation variables
    uint64_t bucketStartTime = 0;
    size_t bucketIndex = 0;
    size_t k = 0;
    struct Metric *m = NULL;

    struct Bucket buckets[DPDK_INST_BUCKET_COUNT];
    memset(&buckets, 0, sizeof(struct Bucket) * DPDK_INST_BUCKET_COUNT);
    for(k = 0; k < DPDK_INST_BUCKET_COUNT; ++k) {
        buckets[k].packetCount.min = UINT64_MAX;
        buckets[k].burstFoundDuration.min = UINT64_MAX;
        buckets[k].noneFoundDuration.min = UINT64_MAX;
        buckets[k].callbackDuration.min = UINT64_MAX;
        buckets[k].packetFreeDuration.min = UINT64_MAX;
    }

    // sprintf buffer
    char scratch[DPDK_INST_SCRATCH_SIZE];
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
    RTE_LOG(INFO, STREAMS_SOURCE, "TSC Hz = %lu\n", rte_get_tsc_hz());

#ifdef DPDK_INST_ENABLE
    // Get the timestamp we started the first bucket
    DPDK_INST_TS(ts_A);
    bucketStartTime = ts_A;
#endif

    while(1) {
        // Read rx packets from the queue.
        for (i = 0; i < conf->num_rx_queue; i++) {
            portid = conf->rx_queue_list[i].port_id;
            streams_packet_cb_t packetCallback = conf->rx_queue_list[i].packetCallbackFunction;
            num_rx = rte_eth_rx_burst((uint32_t) portid, conf->rx_queue_list[i].queue_id,
                                      pkts_burst, MAX_PKT_BURST);
            DPDK_INST_TS(ts_B);

            if (num_rx) {
                DPDK_INST_UPDATE_METRIC(burstFoundDuration, ts_B - ts_A);
                DPDK_INST_UPDATE_METRIC(packetCount, num_rx);

                for (j = 0; j < PREFETCH_OFFSET && j < num_rx; j++) {
                    rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));
                }

                for (j = 0; j < (num_rx - PREFETCH_OFFSET); j++) {
                    rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j + PREFETCH_OFFSET], void *));

                    DPDK_INST_TS(ts_C);
                    if(packetCallback) {
                        packetCallback(conf->rx_queue_list[i].userData,
                                       rte_pktmbuf_mtod(pkts_burst[j], char *),
                                       pkts_burst[j]->data_len, rte_rdtsc());
                    }
                    DPDK_INST_TS(ts_D);
                    rte_pktmbuf_free(pkts_burst[j]);
                    DPDK_INST_TS(ts_E);

                    DPDK_INST_UPDATE_METRIC(callbackDuration, ts_D - ts_C);
                    DPDK_INST_UPDATE_METRIC(packetFreeDuration, ts_E - ts_D);

                    count++;
                }

                for (; j < num_rx; j++) {
                    DPDK_INST_TS(ts_C);
                    if(packetCallback) {
                        packetCallback(conf->rx_queue_list[i].userData,
                                       rte_pktmbuf_mtod(pkts_burst[j], char *),
                                       pkts_burst[j]->data_len, rte_rdtsc());
                    }
                    DPDK_INST_TS(ts_D);
                    rte_pktmbuf_free(pkts_burst[j]);
                    DPDK_INST_TS(ts_E);

                    DPDK_INST_UPDATE_METRIC(callbackDuration, ts_D - ts_C);
                    DPDK_INST_UPDATE_METRIC(packetFreeDuration, ts_E - ts_D);

                    count++;
                }
            } else {
                DPDK_INST_UPDATE_METRIC(noneFoundDuration, ts_B - ts_A);
            }
        }

#ifdef DPDK_INST_ENABLE
        // Check to see if we should dump and reset the buckets
        DPDK_INST_TS(ts_F);

        if(ts_F - bucketStartTime >= bucketTargetSize) {
            // This bucket is finished.

            buckets[bucketIndex].duration = ts_F - bucketStartTime;

            // Move to the next bucket, unless we're out of buckets.
            ++bucketIndex;
            if(bucketIndex == DPDK_INST_BUCKET_COUNT) {
                // Out of buckets!
                // Now we really have to waste a bunch of time, dumping the buckets and clearing them.

                // Use this time to display only.
                struct timespec ts_real;
                clock_gettime(CLOCK_REALTIME, &ts_real);

                for(k = 0; k < DPDK_INST_BUCKET_COUNT; ++k) {
                    // Display the data for this bucket/thread to stdout for now
                    size_t offset = 0;
                    offset += snprintf(scratch + offset, DPDK_INST_SCRATCH_SIZE - offset, "RXTX: receive_loop: METRICS %u %d %d %lu.%09lu-%lu %lu %lu",
                                     lcore_id, port_id, queue_id, ts_real.tv_sec, ts_real.tv_nsec, (uint64_t)DPDK_INST_BUCKET_COUNT - k - 1, buckets[k].priorWastedTime, buckets[k].duration);
                    offset += snprintf(scratch + offset, DPDK_INST_SCRATCH_SIZE - offset, " : 0 0 0 0 0");
                    for(m = &buckets[k].packetCount; m <= &buckets[k].packetFreeDuration; ++m) {
                        offset += snprintf(scratch + offset, DPDK_INST_SCRATCH_SIZE - offset, " : %lu %lu %lu %lu %lu",
                                           m->count, m->total, m->squareTotal, m->min, m->max);
                    }
                    puts(scratch);
                }

                // Reset the counters for next time around, other than the starttime and wasted time, which will be updated after this and kept.
                bucketIndex = 0;
                memset(&buckets, 0, sizeof(struct Bucket) * DPDK_INST_BUCKET_COUNT);
                for(k = 0; k < DPDK_INST_BUCKET_COUNT; ++k) {
                    buckets[k].packetCount.min = UINT64_MAX;
                    buckets[k].burstFoundDuration.min = UINT64_MAX;
                    buckets[k].noneFoundDuration.min = UINT64_MAX;
                    buckets[k].callbackDuration.min = UINT64_MAX;
                    buckets[k].packetFreeDuration.min = UINT64_MAX;
                }

                // Reset the start time for the next bucket, and capture how much time we've wasted.
                DPDK_INST_TS(ts_G);
                buckets[0].priorWastedTime = ts_G - ts_F;
                ts_A = ts_G;
            } else {
                ts_A = ts_F;
            }
            bucketStartTime = ts_A;
        } else {
            ts_A = ts_F;
        }
#endif
    }
}

