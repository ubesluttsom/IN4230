/*
 * Implementation of of the MIP-ARP protocol. I figured this should be seperate
 * from `mip_daemon`.
 */
#ifndef _MIP_ARP_H
#define _MIP_ARP_H

#include "mip.h"
#include "ether.h"

#define MIP_ARP_REQ 0x0
#define MIP_ARP_RES 0x1

#define MIP_CACHE_MISS -36      /* Return value if MIP address not in cache */

/* Defining the MIP-ARP SDU */
struct mip_arp {
        uint32_t type:1, addr:8;
} __attribute__((packed));

/* Map between MIP and hardware addresses. A linked list.
   `next` points to the next mapping, if it exists, else NULL. */
struct mip_cache {
        struct mip_cache *next; /* pointer to next entry in cache */
        uint8_t mip;            /* MIP address */
        uint8_t mac[6];         /* MAC address of destination IF */
        struct sockaddr_ll addr;        /* the address from getifaddrs() */
        int raw_socket;         /* <-- this is a memory waste, since it the
                                   same for all entries, but very convienient */
};

void init_mip_cache(uint8_t, struct mip_cache *, int);
int in_mip_cache(uint8_t, struct mip_cache *);
int handle_arp_packet(struct mip_msg *, struct mip_cache *,
                      struct ether_frame *, struct sockaddr_ll *);
int handle_arp_response(struct mip_arp *, struct mip_cache *,
                        struct sockaddr_ll *);
int send_arp_response(struct mip_msg *, struct mip_cache *,
                      struct ether_frame *, struct sockaddr_ll *);
struct mip_msg *send_arp_request(uint8_t, struct mip_cache *);

#endif                          /* _MIP_ARP_H */
