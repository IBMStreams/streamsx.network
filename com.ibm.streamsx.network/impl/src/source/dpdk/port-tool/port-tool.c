/*********************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * All Rights Reserved
 *********************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <string.h>
#include <error.h>

#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_bus_pci.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_string_fns.h>





int main(int argc, char *argv[]) {
    // Disable core dumps for this tool.  rte_eal_init() core dumps in places that are not what I want.
    struct rlimit rlim;
    int rc = getrlimit(RLIMIT_CORE, &rlim);
    if(rc < 0) {
        error(255, errno, "getrlimit(RLIMIT_CORE) failed");
    } else {
        rlim.rlim_cur = 0;
        rc = setrlimit(RLIMIT_CORE, &rlim);
        if(rc < 0) {
            error(255, errno, "setrlimit(RLIMIT_CORE) failed");
        }
    }

    // initialize DPDK. No args are really needed, but if we pass any into this tool, just pass them along to DPDK.
    argv[0] = "dpdk";
    rc = rte_eal_init(argc, argv);
    if (rc < 0) {
        error(255, 0, "rte_eal_init() failed, rte_errno=%d, %s", rte_errno, rte_strerror(rte_errno));
    }


    struct rte_eth_dev_info dev_info;
    struct ether_addr eth_addr;
    struct rte_eth_link link;

    unsigned port_id;
    uint32_t num_ports = rte_eth_dev_count();
    uint32_t if_index;
    char     ifname[IF_NAMESIZE];


    printf("%d EAL ports available.\n", num_ports);

    for (port_id = 0; port_id < num_ports; port_id++) {
        rte_eth_dev_info_get(port_id, &dev_info);
        rte_eth_macaddr_get(port_id, &eth_addr);
        rte_eth_link_get(port_id, &link);

        if_index = dev_info.if_index;
        if((if_index >= 0) && (if_indextoname(if_index, ifname))) {
        } else {
            strcpy(ifname, "<none>");
        }

        printf("port %d interface-index: %d\n", port_id, if_index);
        printf("port %d interface: %s\n", port_id, ifname);
        printf("port %d driver: %s\n", port_id, dev_info.driver_name);
        printf("port %d mac-addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
               port_id,
               eth_addr.addr_bytes[0],
               eth_addr.addr_bytes[1],
               eth_addr.addr_bytes[2],
               eth_addr.addr_bytes[3],
               eth_addr.addr_bytes[4],
               eth_addr.addr_bytes[5]);
        printf("port %d pci-bus-addr: %04x:%02x:%02x.%x\n",
               port_id,
               dev_info.pci_dev->addr.domain,
               dev_info.pci_dev->addr.bus,
               dev_info.pci_dev->addr.devid,
               dev_info.pci_dev->addr.function);
        printf("port %d socket: %d\n", port_id, rte_eth_dev_socket_id(port_id));

        if (link.link_status) {
            printf("port %d link-state: up\n", port_id);
            printf("port %d link-speed: %u\n", port_id, (unsigned)link.link_speed);
            if(link.link_duplex == ETH_LINK_FULL_DUPLEX) {
                printf("port %d link-duplex: full-duplex\n", port_id);
            } else {
                printf("port %d link-duplex: half-duplex\n", port_id);
            }
        } else {
            printf("port %d link-state: down\n", port_id);
            printf("port %d link-speed: -1\n", port_id);
            printf("port %d link-duplex: <n/a>\n", port_id);
        }


        printf("\n");
    }

    return 0;
}

