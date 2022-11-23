/*
 * Definition of the MIP protocol
 */

#ifndef _MIP_H
#define _MIP_H

#include <stdint.h>             /* uint8_t, uint16_t, uint32_t */
#include <linux/if_packet.h>    /* AF_PACKET, struct sockaddr_ll */

/* EtherType protocol */
#define ETH_P_MIP 0x88B5

/* MIP types */
#define MIP_T_ARP     0x01
#define MIP_T_PING    0x02
#define MIP_T_ROUTING 0x04

#define MIP_BROADCAST 0xFF

/* MIP Header */
struct mip_hdr {
        uint8_t dst_addr;
        uint8_t src_addr;
        uint16_t ttl:4, sdu_len:9, sdu_type:3;
};

/* MIP PDU (or MIP message) */
struct mip_msg {
        struct mip_hdr hdr;
        uint32_t sdu[];
};

/* Both of the structs above should probably be `__attribute__((packed))`, but
 * at the moment it works fine anyway, since we're running og homogeneous
 * systems */

#endif                          /* _MIP_H */
