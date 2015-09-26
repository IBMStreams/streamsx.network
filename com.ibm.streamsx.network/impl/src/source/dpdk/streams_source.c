/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_lcore.h>

#include "init.h"
#include "rxtx.h"
#include "config.h"

#include "streams_source.h"

#define ARGN 7

pthread_mutex_t mutexInit;
int32_t  initComplete = 0;
uint32_t portCount    = 0;

static int main_loop(__attribute__((unused)) void *arg) {
    struct lcore_conf *conf;
    unsigned lcore_id;

    lcore_id = rte_lcore_id();
    conf = &lcore_conf_[lcore_id];
    if (conf->num_rx_queue > 0) {
	receive_loop(conf);
    }

    return 0;
}

uint64_t streams_source_get_tsc_hz(void) {
    return(rte_get_tsc_hz()); 
}

/*
 * This function will be called once for each DPDK Streams operator that
 * is instantiated.  Because it is called from the ctor path, the calls
 * should always be serialized as they run on a single thread.  However,
 * to be safe a mutex is used here to ensure the calls are in fact serialized.
 */
int streams_source_init(const char *pmdDriver, const char *coreMask, 
	streams_packet_cb_t callback, int lcore, int nicPort, int nicQueue,
	int promiscuous, void *user) {
    int ret;
    pthread_mutex_lock(&mutexInit);   
    if(initComplete > 0) {
	// Not the first thread through the init code
	portCount++;

	lcore_conf_[lcore].num_rx_queue = 1;
	lcore_conf_[lcore].rx_queue_list[nicQueue].port_id = nicPort;
	lcore_conf_[lcore].rx_queue_list[nicQueue].queue_id = nicQueue; // TODO redundant
	lcore_conf_[lcore].rx_queue_list[nicQueue].packetCallbackFunction = callback;
	lcore_conf_[lcore].rx_queue_list[nicQueue].userData = user;

	pthread_mutex_unlock(&mutexInit); 
	return 0;
    }

    if(initComplete < 0) {
	// Not the first thread through the init code, and the first
	// one failed.
	pthread_mutex_unlock(&mutexInit); 
	return -1;
    }

    pthread_mutex_unlock(&mutexInit); 
    initComplete = 1;

    lcore_conf_[0].num_rx_queue = 0; // TODO make a param
    lcore_conf_[lcore].num_rx_queue = 1;
    lcore_conf_[lcore].rx_queue_list[nicQueue].port_id = nicPort;
    lcore_conf_[lcore].rx_queue_list[nicQueue].queue_id = nicQueue; // TODO redundant
    lcore_conf_[lcore].rx_queue_list[nicQueue].packetCallbackFunction = callback;
    lcore_conf_[lcore].rx_queue_list[nicQueue].userData = user;

    portCount++;

    uint32_t nb_ports;
    char *rte_arg[ARGN];

    char arg0[]="dpdk";
    char arg1[]="-c";
    char arg2[MAX_COREMASK];
    strncpy(arg2, coreMask, MAX_COREMASK);
    char arg3[]="-n";
    char arg4[]="4";
    char arg5[]="-d";
    char arg6[MAX_COREMASK];
    strncpy(arg6, pmdDriver, MAX_COREMASK);

    rte_arg[0]=arg0;
    rte_arg[1]=arg1;
    rte_arg[2]=arg2;
    rte_arg[3]=arg3;
    rte_arg[4]=arg4;
    rte_arg[5]=arg5;	
    rte_arg[6]=arg6;	

    optind = 0; // Reset getopt state as it is called again in rte_eal_init.

    // This function is to be executed on the master lcore only.
    // Because no master core is specified, the default master_lcore is set by
    // the DPDK library to be the first core in the coremask.
    ret = rte_eal_init(ARGN, rte_arg);
    if (ret < 0) {
	rte_panic("Cannot init EAL\n");
    }

#ifdef RTE_LIBRTE_TIMER
    rte_timer_subsystem_init();
#endif

    rte_set_log_level(8); 

    nb_ports = rte_eth_dev_count();
    if (nb_ports == 0)
	rte_panic("No eth dev found.\n");

    if (init(promiscuous) < 0) {
	rte_panic("Error initializing dpdk subsystem.\n");
    }

    return 0;
}

int streams_source_start(void) {
    int rc = rte_eal_mp_remote_launch(main_loop, NULL, SKIP_MASTER);
    return rc;
}

int streams_port_stats(int nicPort, struct port_stats *outStats) {
    struct rte_eth_stats rteStats;

    rte_eth_stats_get(nicPort, &rteStats);

    outStats->received = rteStats.ipackets;
    outStats->dropped  = rteStats.ierrors;
    outStats->bytes    = rteStats.ibytes;

    return 0;
}

