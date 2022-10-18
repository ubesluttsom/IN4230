/*
 * TODO: finish this, perhaps
 */
#include "mip.h"

void fill_mip_msg(struct mip_msg *msg,
                  uint8_t *dst_mac_addr,
                  uint8_t *src_mac_addr,
                  uint8_t dst_mip_addr,
                  uint8_t src_mip_addr
                  uint8_t ttl,
                  size_t *sdu_len,
                  uint32_t *sdu)

        struct mip_hdr *hdr = &msg->hdr;
        struct mip_arp *arp = (struct mip_arp *)msg->sdu;
        ethhdr.eth_proto = htons(ETH_P_MIP);

        struct ether_frame ethhdr;

        /* Fill in Ethernet header */
        memcpy(ethhdr.dst_addr, dst_mac_addr, 6);
        memcpy(ethhdr.src_addr, src_mac_addr, 6);

        /* Modify MIP message header */
        hdr->dst_addr = dst_mip_addr;
        hdr->src_addr = src_mip_addr;
        hdr->ttl = 0;           /* FIXME: remove hardcoded */
        hdr->sdu_len = sizeof(struct mip_arp) / 4;
        hdr->sdu_type = MIP_T_ARP;
        msg->hdr = *hdr;
}

void free_mip_msg()
{
}

size_t serialize_mip_msg()
size_t deserialize_mip_msg()

/* send_mip_packet() */
