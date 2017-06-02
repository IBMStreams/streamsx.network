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
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>
#include <semaphore.h>
#include <pwd.h>

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

// TODO : make the port# use the interface name

pthread_mutex_t mutexInit = PTHREAD_MUTEX_INITIALIZER;

int32_t dpdkInitComplete = 0;
int32_t numOperators = 0;
int32_t firstInitComplete = 0;
int32_t numOperatorsReady = 0;

// Assume maximum of 4 digits+comma
#define MAX_CORESTRING RTE_MAX_LCORE*5+1
char lcoreList[MAX_CORESTRING] = "";

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
 * This function is called by each Streams PacketDPDKSource operator in its constructor.
 * There is no architectural requirement that the constructors run serially or in any specific
 * order, so this code uses a mutex to do some basic initialization once on the first call.
 * The remaining calls all update operator specific data structures.  Note that by implementation
 * the Streams runtime does serialize the operator constructors.
 *
 *    lcore    : Logical core number (hardware thread) on which to run.
 *
 *    callback : Pointer to the Streams operator function to call for packet processing.
 *
 *    user     : Pointer to the this pointer of the Streams operator.
 *
 *  Return Values: 0 on success, -1 on failure.
 *
 */
int streams_operator_init(int lcoreMaster, int lcore, int nicPort, int nicQueue,
                          int promiscuous, streams_packet_cb_t dpdkCallback, void *user) {

    pthread_mutex_lock(&mutexInit);   
    printf("STREAMS_SOURCE: streams_operator_init(lcoreMaster=%d, lcore=%d, nicPort=%d, nicQueue=%d, promiscuous=%d, ...) starting ...\n", lcoreMaster, lcore, nicPort, nicQueue, promiscuous); 
    
    if(firstInitComplete == 0) {
        const char *homeDir = getenv("HOME");
        const uid_t uid = getuid(); 
        const char *newHomeDir = getpwuid(getuid())->pw_dir;
        if(newHomeDir) setenv("HOME", newHomeDir, 1); 
        printf("STREAMS_SOURCE: UID=%d, Original Home=%s, New Home=%s.\n", 
            (int)uid, homeDir, newHomeDir); 
        // Initialize data structures.
        coreMaster_   = -1;
        numQueues_    =  0;
        maxPort_      = -1;
        numOperators  = 0;

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

        // Add the lcore for this operator to the lcore list.
        sprintf(lcoreList, "%d", lcore); 

        firstInitComplete = 1;
    } else {
        // Add the lcore for this operator to the lcore list.
        char coreStr[6];
        snprintf(coreStr, 6, ",%d", lcore); 
        strcat(lcoreList, coreStr); 
    }

    if((coreMaster_ == -1) && (lcoreMaster >= 0)) {
        // If no lcore master has been set by any operator, and this one has set a
        // value, use it.
        coreMaster_ = lcoreMaster;
        char coreStr[6];
        snprintf(coreStr, 6, ",%d", lcoreMaster); 
        strcat(lcoreList, coreStr); 
        printf("STREAMS_SOURCE: streams_operator_init: coreMaster set to lcore %d.\n", 
            coreMaster_); 
    } else if((coreMaster_ != lcoreMaster) && (lcoreMaster >= 0)) {
        // The lcore master has been set and this operator has specified a value.
        // Do not use the new value, but log a warning.
        printf("STREAMS_SOURCE: streams_operator_init: coreMaster not changed from lcore %d.\n", 
            coreMaster_); 
    }

    if(nicPort >= MAX_PORTS) {
        printf("STREAMS_SOURCE: Error - Invalid NIC port value %d, max of %d can be used.\n", 
               nicPort, MAX_PORTS);
        pthread_mutex_unlock(&mutexInit);
        return(-1);
    }
    
    if(lcore >= RTE_MAX_LCORE) {
        printf("STREAMS_SOURCE: Error - Invalid lcore value %d, max of %d can be used.\n", 
               lcore, RTE_MAX_LCORE);
        pthread_mutex_unlock(&mutexInit);
        return(-1);
    }
    
    int coreQueue = lcore_conf_[lcore].num_rx_queue;
    lcore_conf_[lcore].rx_queue_list[coreQueue].port_id = nicPort;
    lcore_conf_[lcore].rx_queue_list[coreQueue].queue_id = nicQueue;
    lcore_conf_[lcore].rx_queue_list[coreQueue].userData = user;
    lcore_conf_[lcore].rx_queue_list[coreQueue].packetCallbackFunction = dpdkCallback;
    lcore_conf_[lcore].num_rx_queue += 1;
    if(lcore_conf_[lcore].num_rx_queue == MAX_RX_QUEUE_PER_LCORE) {
        printf("STREAMS_SOURCE: Error - Invalid number of queues on lcore %d, only %d can be defined.\n", lcore, MAX_RX_QUEUE_PER_LCORE);
        pthread_mutex_unlock(&mutexInit);
        return(-1);
    }

    if(promiscuous) port_info_[nicPort].promiscuous = 1;

    // Set the number of queues for all NICs equal to the largest queue value that any
    // operator defines.  This could be made more granular and be done by NIC port in a
    // future code update.
    if((nicQueue+1) > numQueues_) {
        numQueues_  = nicQueue+1;
    }

    if(nicPort > maxPort_) {
        maxPort_ = nicPort;
    }

    printf("STREAMS_SOURCE: lcore = %d, coreQueue = %d, nicPort = %d, nicQueue = %d.\n",
        lcore, coreQueue, nicPort, nicQueue);
    printf("STREAMS_SOURCE: promiscuous = %d, callback = 0x%lx, user = 0x%lx.\n",
        promiscuous, dpdkCallback, user);


    printf("STREAMS_SOURCE: lcoreList = %s\n", lcoreList); 

    numOperators++;  // Keep a count of the number of operators that are constructed.

    printf("STREAMS_SOURCE: ... streams_operator_init(...) finished\n"); 

    pthread_mutex_unlock(&mutexInit); 
    return(0); 
}

/*
 *  Initialize the DPDK library.
 */
int streams_dpdk_init() {
    pthread_mutex_lock(&mutexInit);   
    printf("STREAMS_SOURCE: streams_dpdk_init() starting ...\n"); 

    if(dpdkInitComplete != 0) {
	// Not the first thread through the init code so just return.
	pthread_mutex_unlock(&mutexInit); 
        return(0);
    }

    if((numOperators == 0) || (coreMaster_ == -1) || 
       (numQueues_ == 0) || (maxPort_ == -1)) {
        printf("STREAMS_SOURCE: streams_dpdk_init aborting due to invalid configuration.\n"); 
        printf("STREAMS_SOURCE: numOperators = %d, coreMaster_ = %d, numQueues_ = %d, maxPort_ = %d\n", 
               numOperators, coreMaster_, numQueues_, maxPort_); 
	pthread_mutex_unlock(&mutexInit); 
        return(-1);
    }

    uint32_t nb_ports;
    char *rte_arg[ARGN];

    char arg0[]="dpdk";
    char arg1[]="-l";
    char arg2[MAX_CORESTRING];
    strcpy(arg2, lcoreList); 
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

    printf("STREAMS_SOURCE: Queues per port = %d, Number of operators = %d.\n", 
           numQueues_, numOperators);
    printf("STREAMS_SOURCE: streams_dpdk_init() calling rte_eal_init(%s %s %s %s %s %s %s)\n", 
           arg0, arg1, arg2, arg3, arg4, arg5, arg6);

    optind = 0; // Reset getopt state as it is called again in rte_eal_init.

    // This function is to be executed on the master lcore only.
    // Because no master core is specified, the default master_lcore is set by
    // the DPDK library to be the first core in the coremask.
    unsigned int ret = rte_eal_init(ARGN, rte_arg);
    if (ret < 0) {
        pthread_mutex_unlock(&mutexInit);
	rte_panic("Cannot init EAL ....... rte_eal_init() failed\n");
        return(-1);
    }

#ifdef RTE_LIBRTE_TIMER
    rte_timer_subsystem_init();
#endif

    rte_set_log_level(8); 

    nb_ports = rte_eth_dev_count();
    if (nb_ports == 0) {
        pthread_mutex_unlock(&mutexInit);
        RTE_LOG(ERR, STREAMS_SOURCE, "No ethernet device found.\n");
        return(-1);
    }
    if (maxPort_ > nb_ports) {
        pthread_mutex_unlock(&mutexInit);
        RTE_LOG(ERR, STREAMS_SOURCE, "A PacketDPDKSource operator specified an invalid port.\n");
        return(-1);
    }

    printf("STREAMS_SOURCE: ... streams_dpdk_init() finished\n"); 

    dpdkInitComplete = 1;
    pthread_mutex_unlock(&mutexInit);

    return(0);
}

/*
 * This function starts a dpdk receive thread on each lcore.  That thread takes in packets
 * and calls the Streams operator callback function for processing.
 *
 * Each PacketDPDKSource operator calls this function once its constructor is complete 
 * and a source operator thread has been started.  That Streams thread is then held in
 * a loop in the operator until it is shut down.  Once all of the operators have called this
 * function and are ready, we can start the dpdk threads on all lcores.
 */
int streams_source_start(void) {
    pthread_mutex_lock(&mutexInit);   
    numOperatorsReady++;
    if(numOperatorsReady < numOperators) {
        // Not all operator threads are ready, so this one can return and enter its
        // spin loop.
        pthread_mutex_unlock(&mutexInit);     
        return(0); 
    }

    pthread_mutex_unlock(&mutexInit);     
    if(init() < 0) {
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

