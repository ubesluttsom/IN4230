/*
 * File: mip.h
 *
 * Good practises for C header files.
 * 1. ALWAYS #include guards!
 * 2. #include only necessary libraries!
 * 3. DO NOT define global variables!
 *    Are you sure you need them? There might be a better way.
 *    If you still need them, make sure to employ the `extern` keyword.
 * 4. #define MACROs and declare functions prototype to be shared between
 *    .c source files.
 *
 * Check https://valecs.gitlab.io/resources/CHeaderFileGuidelines.pdf for some
 * more nice practises.
 */

/*
 * #ifndef and #define are known as header guards.
 * Their primary purpose is to prevent header files
 * from being included multiple times.
 */

#ifndef _MIP_H
#define _MIP_H

#include <stdint.h>   /* uint8_t, uint16_t, uint32_t */
#include <linux/if_packet.h>   /* AF_PACKET, struct sockaddr_ll */

/* MACROs */

// Protocol
#define ETH_P_MIP 0x88B5

// MIP types
#define MIP_T_ARP   0x01
#define MIP_T_PING  0x02

#define MIP_BROADCAST 0xFF

// TODO: Add stuff.

// MIP Header

struct mip_hdr {
  uint8_t  dst_addr;
  uint8_t  src_addr;
  uint16_t ttl      : 4,
           sdu_len  : 9,
           sdu_type : 3;
};

struct mip_msg {
  struct mip_hdr hdr;
  uint32_t       sdu[];
};

#endif /* _MIP_H */
