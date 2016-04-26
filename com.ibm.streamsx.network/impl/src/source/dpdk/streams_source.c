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
#include <semaphore.h>

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
int numOperatorsNotReady;

int32_t  initComplete = 0;
uint32_t portCount    = 0;

/*
 * This is the entry point for the spin loop that is started on each lcore
 * for ingesting packet data from the DPDK libraries.
 */
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

/*
 *  Initialize the master thread.
 *
 *  Parameters:
 *    coreMask : Hex string representing the lcores to be used for DPDK threads.
 *               A maximum of 32 hex characters can be used for 128 lcores.
 *
 *    portMask : Hex string representing the ports to be used for DPDK.
 *
 *    numQueues: Number of queues configured for each port.  This value is defined to
 *               be constant across all ports being utilized.
 *
 *    lcore    : Logical core number (hardware thread) on which to run.
 *
 *    callback : Pointer to the Streams operator function to call for packet processing.
 *
 *    user     : Pointer to the this pointer of the Streams operator.
 *
 *  Return Values: 0 on success, -1 on failure.
 *
 *  Notes: The total number of Streams operators that must be instantiated is equal to 
 *         1 master + numPorts * numQueues.  If too few operators are instantiated, the
 *         application will terminate.
 */
int streams_master_init(const char* coreMask, const char* portMask, int numQueues,
                        int lcore, streams_packet_cb_t callback, void *user) {

    pthread_mutex_lock(&mutexInit);   
    printf("STREAMS_SOURCE: streams_master_init starting.\n"); 

    if(initComplete != 0) {
	// Not the first master thread through the init code.
        printf("STREAMS_SOURCE: Error - streams_master_init called twice.\n");
        printf("STREAMS_SOURCE: Only one master DPDK operator can be created.\n");
	pthread_mutex_unlock(&mutexInit); 
        return(-1);
    }

    char *end = NULL;
    coreMask_   = strtoull(coreMask, &end, 16);
    if ((coreMask[0] == '\0') || (end == NULL) || (*end != '\0')) {
        printf("STREAMS_SOURCE: Error - Invalid core mask.\n");
        return(-1);
    }

    end = NULL;
    portMask_   = strtoull(portMask, &end, 16);
    if ((portMask[0] == '\0') || (end == NULL) || (*end != '\0')) {
        printf("STREAMS_SOURCE: Error - Invalid port mask.\n");
        return(-1);
    }

    numQueues_  = numQueues;
    coreMaster_ = lcore;
    numPorts_   = __builtin_popcount(portMask_); 
    numOperators_ = (numPorts_ * numQueues_) + 1;
    numOperatorsNotReady = numOperators_;

    int lcore_id;
    for(lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        lcore_conf_[lcore_id].num_rx_queue = 0;
        lcore_conf_[lcore_id].socket_id = 0;
        lcore_conf_[lcore_id].num_frames = 0;
        lcore_conf_[lcore_id].ring = NULL;

        int port;
        for(port = 0; port < MAX_PORTS; port++) {
            lcore_conf_[lcore_id].tx_queue_id[port] = 0;
        }

        int rxq;
        for(rxq = 0; rxq < MAX_RX_QUEUE_PER_LCORE; rxq++) {
            lcore_conf_[lcore_id].rx_queue_list[rxq].port_id = 0;
            lcore_conf_[lcore_id].rx_queue_list[rxq].queue_id = 0;
            lcore_conf_[lcore_id].rx_queue_list[rxq].userData = NULL;
            lcore_conf_[lcore_id].rx_queue_list[rxq].packetCallbackFunction = NULL;
        }
    }

    int port_id;
    for(port_id = 0; port_id < MAX_PORTS; port_id++) {
        port_info_[port_id].promiscuous = 0;
    }

    uint32_t nb_ports;
    char *rte_arg[ARGN];

    char arg0[]="dpdk";
    char arg1[]="-c";
    char arg2[MAX_COREMASK];
    sprintf(arg2, "0x%llx", coreMask_); 
    char arg3[]="-n";
    char arg4[]="4";
    char arg5[]="--master-lcore";
    char arg6[16];
    sprintf(arg6, "%d", coreMaster_); 

    rte_arg[0]=arg0;
    rte_arg[1]=arg1;
    rte_arg[2]=arg2;
    rte_arg[3]=arg3;
    rte_arg[4]=arg4;
    rte_arg[5]=arg5;
    rte_arg[6]=arg6;

    printf("STREAMS_SOURCE: coreMask = 0x%llx, portMask = 0x%llx.\n", coreMask_, portMask_);
    printf("STREAMS_SOURCE: numQueues = %d, lcore = %d.\n", numQueues, lcore);
    printf("STREAMS_SOURCE: callback = 0x%lx, user = 0x%lx.\n", callback, user);
    printf("STREAMS_SOURCE: args = %s %s %s %s %s.\n", arg0, arg1, arg2, arg3, arg4);

    optind = 0; // Reset getopt state as it is called again in rte_eal_init.

    // This function is to be executed on the master lcore only.
    // Because no master core is specified, the default master_lcore is set by
    // the DPDK library to be the first core in the coremask.
    unsigned int ret = rte_eal_init(ARGN, rte_arg);
    if (ret < 0) {
        pthread_mutex_unlock(&mutexInit);
	rte_panic("Cannot init EAL\n");
        return(-1);
    }

#ifdef RTE_LIBRTE_TIMER
    rte_timer_subsystem_init();
#endif

    rte_set_log_level(8); 

    nb_ports = rte_eth_dev_count();
    if (nb_ports == 0) {
        pthread_mutex_unlock(&mutexInit);
	rte_panic("No eth dev found.\n");
        return(-1);
    }

    initComplete = 1;
    pthread_mutex_unlock(&mutexInit);

    return(0);
}

/*
 *  Initialize a slave thread.
 *
 *  Parameters:
 *    lcore    : Logical core number (hardware thread) on which to run.
 *    nicPort  : Port number of the NIC 
 *    nicQueue : Logical core number (hardware thread) on which to run.
 *
 *    callback : Pointer to the Streams operator function to call for packet processing.
 *
 *    user     : Pointer to the this pointer of the Streams operator.
 *
 *  Return Values: 0 on success, -1 on failure.
 *
 *  Notes: The total number of Streams operators that must be instantiated is equal to 
 *         1 master + numPorts * numQueues.  If too few operators are instantiated, the
 *         application will terminate.
 */
int streams_slave_init(int lcore, int nicPort, int nicQueue, int promiscuous,
                       streams_packet_cb_t callback, void *user) {

    // Wait until a master thread completes initialization.
    int timeoutCount = 0;
    while(!initComplete) {
        sleep(1);
        timeoutCount++;
        if(timeoutCount == 30) {
            printf("STREAMS_SOURCE: Error - streams_slave_init timeout.\n");
            return(-1);
        }
    }

    pthread_mutex_lock(&mutexInit);   
    printf("STREAMS_SOURCE: streams_slave_init starting.\n"); 

    if((coreMask_ & (0x1 << ((unsigned long long)lcore)) == 0)) {
        printf("STREAMS_SOURCE: Error - Invalid lcore %d.\n", lcore);
        pthread_mutex_unlock(&mutexInit); 
        return(-1);
    }
    if((portMask_ & (0x1 << ((unsigned long long)nicPort))) == 0) {
        printf("STREAMS_SOURCE: Error - Invalid nicPort %d.\n", nicPort);
        pthread_mutex_unlock(&mutexInit); 
        return(-1);
    }
    if(nicQueue >= numQueues_) {
        printf("STREAMS_SOURCE: Error - Invalid nicQueue %d.\n", nicQueue);
        pthread_mutex_unlock(&mutexInit); 
        return(-1);
    }

    int coreQueue = lcore_conf_[lcore].num_rx_queue;
    lcore_conf_[lcore].rx_queue_list[coreQueue].port_id = nicPort;
    lcore_conf_[lcore].rx_queue_list[coreQueue].queue_id = nicQueue;
    lcore_conf_[lcore].rx_queue_list[coreQueue].userData = user;
    lcore_conf_[lcore].rx_queue_list[coreQueue].packetCallbackFunction = callback;
    lcore_conf_[lcore].num_rx_queue += 1;
    if(lcore_conf_[lcore].num_rx_queue == MAX_RX_QUEUE_PER_LCORE) {
        printf("STREAMS_SOURCE: Error - Invalid number of queues on lcore %d, only %d can be defined.\n", lcore, MAX_RX_QUEUE_PER_LCORE);
        pthread_mutex_unlock(&mutexInit); 
        return(-1);
    }

    if(promiscuous) port_info_[nicPort].promiscuous = 1;

    printf("STREAMS_SOURCE: lcore = %d, coreQueue = %d, nicPort = %d, nicQueue = %d.\n", 
        lcore, coreQueue, nicPort, nicQueue);
    printf("STREAMS_SOURCE: promiscuous = %d, callback = 0x%lx, user = 0x%lx.\n", 
        promiscuous, callback, user);

    pthread_mutex_unlock(&mutexInit); 
    return 0;
}

/*
 * Each PacketDPDKSource operator, including the master, calls this function
 * once its constructor is complete and a source operator thread has been started.
 */
int streams_source_start(int isMaster) {
    pthread_mutex_lock(&mutexInit);   
    --numOperatorsNotReady;
    pthread_mutex_unlock(&mutexInit);   

    if(!isMaster) {
        return(0); 
    }

    int timeoutCount = 0;
    while(numOperatorsNotReady) {
        sleep(1);
        timeoutCount++;
        if(timeoutCount == 30) {
            printf("STREAMS_SOURCE: Error - streams_source_start timeout.\n");
            return(-1);
        }
    }

    if (init() < 0) {
        return(-1); 
    }

    int rc = rte_eal_mp_remote_launch(main_loop, NULL, SKIP_MASTER);
    return rc;
}

/*
 * Return port statistics to the Streams operator.
 */
int streams_port_stats(int nicPort, struct port_stats *outStats) {
    struct rte_eth_stats rteStats;

    rte_eth_stats_get(nicPort, &rteStats);

    outStats->received = rteStats.ipackets;
    outStats->dropped  = rteStats.ierrors;
    outStats->bytes    = rteStats.ibytes;

    return 0;
}

/*
 * Return the frequency of the TSC/Timebase register.
 */
uint64_t streams_source_get_tsc_hz(void) {
    return(rte_get_tsc_hz()); 
}

