/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 *********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <string.h>

#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_string_fns.h>

#include "config.h"
#include "init.h"

struct rte_mempool *socket_mempool_[MAX_SOCKETS];
struct lcore_conf lcore_conf_[RTE_MAX_LCORE];
struct port_info port_info_[MAX_PORTS];
int maxPort_;
int numQueues_;
int coreMaster_;

static const struct rte_eth_conf port_conf_ = {
    .rxmode = {
	.max_rx_pkt_len = ETHER_MAX_LEN,
	.split_hdr_size = 0,
	.header_split   = 0, /**< Header Split disabled */
	.hw_ip_checksum = 1, /**< IP checksum offload enabled */
	.hw_vlan_filter = 0, /**< VLAN filtering disabled */
	.hw_vlan_extend = 0, /**< Extended VLAN disabled. */
	.jumbo_frame    = 0, /**< Jumbo frame support disabled */
	.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	.mq_mode        = ETH_MQ_RX_RSS,
    },
    .rx_adv_conf = {
	.rss_conf = {
	    .rss_key = NULL,
	    .rss_hf =  ETH_RSS_IPV4 | ETH_RSS_IPV6,
	},
    },
    .txmode = {
	.mq_mode = ETH_DCB_NONE,
    },
};

static const struct rte_eth_rxconf rx_conf_ = {
    .rx_thresh = {
	.pthresh = RX_PTHRESH,
	.hthresh = RX_HTHRESH,
	.wthresh = RX_WTHRESH,
    },
    .rx_free_thresh = 32,
};

static struct rte_eth_txconf tx_conf_ = {
    .tx_thresh = {
	.pthresh = TX_PTHRESH,
	.hthresh = TX_HTHRESH,
	.wthresh = TX_WTHRESH,
    },
    .tx_free_thresh = 0, /* Use PMD default values */
    .tx_rs_thresh = 0,   /* Use PMD detault values */
    .txq_flags = 0x0,
};

/*
 * Create a memory pool for every enabled socket.
 */
static void init_pools(void) {
    unsigned i, j;
    unsigned socket_id;
    struct rte_mempool *mp;

    RTE_LOG(INFO, STREAMS_SOURCE, "init.c init_pools() starting ...\n");

    for (i = 0; i < MAX_SOCKETS; ++i) {
        socket_mempool_[i] = NULL;
    }

    for (i = 0; i < RTE_MAX_LCORE; ++i) {
        if (rte_lcore_is_enabled(i) == 0) {
            continue;
        }

        socket_id = rte_lcore_to_socket_id(i);
        if (socket_id >= MAX_SOCKETS) {
            rte_exit(EXIT_FAILURE, "socket_id %d >= MAX_SOCKETS\n",
                    socket_id);
        }

        if (socket_mempool_[socket_id] != NULL) {
            continue;
        }

        // format a name for the buffer pool
        char s[64];
        sprintf(s, "%d", socket_id);

        // calculate the number of buffers the pool will need
        int queueCount = 0; 
        for (j = 0; j<RTE_MAX_LCORE; j++) {
            if (rte_lcore_is_enabled(j)==0) continue;
            struct lcore_conf *conf = &lcore_conf_[j];
            queueCount += conf->num_rx_queue;
        }
        int bufferCount = 2 * queueCount * STREAMS_SOURCE_RX_DESC_DEFAULT; // ... was ... NB_MBUF;

        // allocate the buffer pool using either rte_pktmbuf_pool_create() or rte_mempool_create()
#if 1
        RTE_LOG(INFO, STREAMS_SOURCE, "init.c init_pools() calling rte_pktmbuf_pool_create(name='%s', bufferCount=%d, cacheSize=%d, privateSize=%d, bufferSize=%d, numaSocket=%d)\n", s, bufferCount, MEMPOOL_CACHE_SIZE, 0, MBUF_SIZE, socket_id);
        mp = rte_pktmbuf_pool_create(s, bufferCount, MEMPOOL_CACHE_SIZE, 0, MBUF_SIZE, socket_id);
        if (mp == NULL) { rte_exit(EXIT_FAILURE, "Error in STREAMS_SOURCE init.c init_pools() calling rte_pktmbuf_pool_create(), rte_errno=%d, %s", rte_errno, rte_strerror(rte_errno)); }
#else
        RTE_LOG(INFO, STREAMS_SOURCE, "init.c init_pools() calling rte_mempool_create(name='%s', bufferCount=%d, bufferSize=%d, cacheSize=%d, privateDataSize=%d, ,,,, numaSocket=%d, flags=0)\n", s, bufferCount, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), socket_id);
        mp = rte_mempool_create(s, bufferCount, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, socket_id, 0);
        if (mp == NULL) { rte_exit(EXIT_FAILURE, "Error in STREAMS_SOURCE init.c init_pools() calling rte_mempool_create(), rte_errno=%d, %s", rte_errno, rte_strerror(rte_errno)); }
#endif
        socket_mempool_[socket_id] = mp;
    }

    RTE_LOG(INFO, STREAMS_SOURCE, "... init.c init_pools() finished\n");

}

static void init_ports(void) {
    int ret;
    uint8_t num_rx_queues, num_tx_queues, socket_id, queue_id;
    unsigned port_id, lcore_id, queue;
    struct lcore_conf *conf;
    struct rte_eth_link link;
    struct rte_eth_dev_info dev_info;
    struct ether_addr eth_addr;

    RTE_LOG(INFO, STREAMS_SOURCE, "init.c init_ports() starting ...\n");

    uint32_t num_ports = rte_eth_dev_count();
    uint32_t if_index;
    char     ifname[IF_NAMESIZE];

    lcore_id = rte_get_master_lcore();
    socket_id = rte_lcore_to_socket_id(lcore_id);
    conf = &lcore_conf_[lcore_id];

    RTE_LOG(INFO, STREAMS_SOURCE, "==========\n");
    if(num_ports) {
        RTE_LOG(INFO, STREAMS_SOURCE, "Port numbers available: 0 to %d\n", num_ports-1);
    } else {
        RTE_LOG(INFO, STREAMS_SOURCE, "Port numbers available: <none>\n");
    }
    for (port_id = 0; port_id < num_ports; port_id++) {
	RTE_LOG(INFO, STREAMS_SOURCE, "Init port %u\n", port_id);
	rte_eth_dev_info_get(port_id, &dev_info);
	rte_eth_macaddr_get(port_id, &eth_addr);

        if_index = dev_info.if_index;
        if((if_index >= 0) && (if_indextoname(if_index, ifname))) {
        } else {
            strcpy(ifname, "<none>"); 
        }
	RTE_LOG(INFO, STREAMS_SOURCE, 
		"  Interface (%d): %s\n", if_index, ifname);

	RTE_LOG(INFO, STREAMS_SOURCE, 
		"  Driver       : %s\n", dev_info.driver_name); 
	RTE_LOG(INFO, STREAMS_SOURCE, 
		"  MAC Addr     : %02x:%02x:%02x:%02x:%02x:%02x\n", 
		eth_addr.addr_bytes[0],
		eth_addr.addr_bytes[1],
		eth_addr.addr_bytes[2],
		eth_addr.addr_bytes[3],
		eth_addr.addr_bytes[4],
		eth_addr.addr_bytes[5]);
	RTE_LOG(INFO, STREAMS_SOURCE, 
		"  PCI Bus Addr : %04x:%02x:%02x:%x\n", 
		dev_info.pci_dev->addr.domain,
		dev_info.pci_dev->addr.bus,
		dev_info.pci_dev->addr.devid,
		dev_info.pci_dev->addr.function);
	RTE_LOG(INFO, STREAMS_SOURCE, 
                "  Socket       : %d\n", 
		rte_eth_dev_socket_id(port_id));
	RTE_LOG(INFO, STREAMS_SOURCE, 
		"  RXQ Max      : %d\n", dev_info.max_rx_queues); 
	RTE_LOG(INFO, STREAMS_SOURCE, 
		"  TXQ Max      : %d\n", dev_info.max_tx_queues); 

	num_rx_queues = numQueues_;
	num_tx_queues = 1;
	RTE_LOG(INFO, STREAMS_SOURCE, 
                "  RXQ Set      : %d\n", num_rx_queues);
	RTE_LOG(INFO, STREAMS_SOURCE,
                "  TXQ Set      : %d\n", num_tx_queues);

        if((num_rx_queues > dev_info.max_rx_queues) || 
           (num_tx_queues > dev_info.max_tx_queues)) {
	    rte_exit(EXIT_FAILURE, "More RX or TX queues requested than this device supports.\n");
        }

	ret = rte_eth_dev_configure(port_id, num_rx_queues, num_tx_queues,
		&port_conf_);
	if (ret < 0) {
	    rte_exit(EXIT_FAILURE, "cannot configure device %u: err=%d\n",
		    port_id, ret);
	}

	/* Initialize tx queues for the port */
	queue_id = 0;
	ret = rte_eth_tx_queue_setup(port_id, queue_id, STREAMS_SOURCE_TX_DESC_DEFAULT, socket_id, &tx_conf_);
	if (ret < 0) {
	    rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: err=%d"
		    " port=%u\n", ret, port_id);
	}

	conf->tx_queue_id[port_id] = queue_id;
        RTE_LOG(INFO, STREAMS_SOURCE, "----------\n");
    }

    /* Initialize rx queues */
    for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {

      // skip this core if its not enabled for DPDK
      if (rte_lcore_is_enabled(lcore_id) == 0) continue;

      conf = &lcore_conf_[lcore_id];
      RTE_LOG(INFO, STREAMS_SOURCE, "Initializing receive queues on lcore %u: number of queues: %d, callback: 0x%lx\n", lcore_id, conf->num_rx_queue, conf->rx_queue_list[0].packetCallbackFunction);
      for (queue = 0; queue < conf->num_rx_queue; queue++) {
	    port_id = conf->rx_queue_list[queue].port_id;
	    queue_id = conf->rx_queue_list[queue].queue_id;
	    socket_id = rte_lcore_to_socket_id(lcore_id);
        RTE_LOG(INFO, STREAMS_SOURCE, "init.c init_ports() calling rte_eth_rx_queue_setup(port_id=%d, rx_queue_id=%d, nb_rx_desc=%d, socket_id=%d, ...)\n", port_id, queue_id, STREAMS_SOURCE_RX_DESC_DEFAULT, socket_id);
	    ret = rte_eth_rx_queue_setup(port_id, queue_id, STREAMS_SOURCE_RX_DESC_DEFAULT, socket_id, &rx_conf_, socket_mempool_[socket_id]);
	    if (ret < 0) {
          printf("STREAMS_SOURCE: rte_eth_rx_queue_setup() failed, rte_errno=%d, %s\n", rte_errno, rte_strerror(rte_errno));
          rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup: err=%d port=%u\n", ret, port_id);
	    }
      }
    }

    /* Start ports */
    for (port_id = 0; port_id < num_ports; port_id++) {

	RTE_LOG(INFO, STREAMS_SOURCE, 
		"Start port %d\n", port_id);
	ret = rte_eth_dev_start(port_id);
	if (ret < 0) {
	    rte_exit(EXIT_FAILURE, 
		    "rte_eth_dev_start: err=%d port=%u\n",
		    ret, port_id);
	}

	rte_eth_link_get(port_id, &link);
	if (link.link_status) {
	    RTE_LOG(INFO, STREAMS_SOURCE, 
		    "  Link up.  Speed %u Mbps %s\n", 
		    (unsigned) link.link_speed,
		    (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
		    ("full-duplex") : ("half-duplex\n")
		   );
	} else {
	    RTE_LOG(INFO, STREAMS_SOURCE, "  Link down.\n");
	}

	if (port_info_[port_id].promiscuous) rte_eth_promiscuous_enable(port_id);
    }

    RTE_LOG(INFO, STREAMS_SOURCE, "... init.c init_ports() finished\n");
}

int init(void) {
    int i;

    RTE_LOG(INFO, STREAMS_SOURCE, "init.c init() starting ...\n");

    /* Assign socket id's */
    for(i = 0; i < RTE_MAX_LCORE; ++i) {
	if (rte_lcore_is_enabled(i) == 0) {
	    continue;
        }
	lcore_conf_[i].socket_id = rte_lcore_to_socket_id(i);
    }

    rte_srand(rte_rdtsc());

    init_pools();
    init_ports();

    RTE_LOG(INFO, STREAMS_SOURCE, "... init.c init() finished\n");

    return(0);
}
