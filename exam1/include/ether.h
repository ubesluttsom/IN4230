#ifndef _ETHER_H
#define _ETHER_H

#include <net/ethernet.h>    /* ETH_* */

#define ETH_BROADCAST {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define ETH_MAC_LEN sizeof(uint8_t) * 6

struct ether_frame {
  uint8_t dst_addr[6];
  uint8_t src_addr[6];
  uint16_t eth_proto;
  uint8_t contents[];
} __attribute__((packed));

#endif /* _ETHER_H */
