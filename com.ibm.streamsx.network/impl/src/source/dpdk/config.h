/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 ********************************************************************/

#ifndef _CONFIG_H__
#define _CONFIG_H__

#include <rte_config.h>
#include <rte_atomic.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ether.h>

#include "streams_source.h"

#ifdef RTE_LOG_LEVEL
#undef RTE_LOG_LEVEL
#endif
#define RTE_LOG_LEVEL 7

#define RTE_LOGTYPE_STREAMS_SOURCE RTE_LOGTYPE_USER1
#define MEMPOOL_CACHE_SIZE 512
#define MBUF_SIZE (4096 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#define NB_MBUF   (unsigned)6384

/*
 * RX and TX Prefetch, Host, and Write-back threshold values should be
 * carefully set for optimal performance. Consult the network
 * controller's datasheet and supporting DPDK documentation for guidance
 * on how these parameters should be set.
 */
#define RX_PTHRESH 8 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 4 /**< Default values of RX write-back threshold reg. */

/*
 * These default values are optimized for use with the Intel(R) 82599 10 GbE
 * Controller and the DPDK ixgbe PMD. Consider using other values for other
 * network controllers and/or network drivers.
 */
#define TX_PTHRESH 36 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 0  /**< Default values of TX write-back threshold reg. */

#define EM_TX_WTHRESH 8  /**< Default values of TX write-back threshold reg. */

#define MAX_RX_QUEUE_PER_LCORE 4
#define MAX_PKT_BURST  32
#define BURST_TX_DRAIN 220000ULL /* around 100us at 2.2 Ghz */

/*
 * Configurable number of RX/TX ring descriptors
 */
#define STREAMS_SOURCE_RX_DESC_DEFAULT 4096
#define STREAMS_SOURCE_TX_DESC_DEFAULT 64
#define STREAMS_SOURCE_MTU_DEFAULT     9000

#define MAX_PORTS 32

#define MAX_SOCKETS 4
#define MAX_OUTPUT 16
#define MAX_COREMASK 255

struct rx_queue {
    uint8_t port_id;
    uint8_t queue_id;
    streams_packet_cb_t packetCallbackFunction;
    void *userData;
};

/*
 * Each lcore owns a queue configuration
 */
struct lcore_conf {
    unsigned num_rx_queue;
    struct rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
    unsigned tx_queue_id[MAX_PORTS];
    unsigned socket_id;
    uint32_t num_frames;
    struct rte_ring *ring;
} __rte_cache_aligned;

struct port_info {
    int promiscuous;
} __rte_cache_aligned;

extern struct lcore_conf lcore_conf_[RTE_MAX_LCORE];
extern struct port_info port_info_[MAX_PORTS];
extern int maxPort_;
extern int numQueues_;
extern int coreMaster_;

#endif
