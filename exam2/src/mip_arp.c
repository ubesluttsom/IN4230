#include "common.h"
#include "mip_arp.h"

#define MIP_ARP_SDU_SIZE sizeof(struct mip_arp)
#define MIP_ARP_MSG_SIZE sizeof(struct mip_msg) + MIP_ARP_SDU_SIZE

#define ARP_ADDR (msg->sdu[0] >> 23) & 0xFF
#define ARP_TYPE (msg->sdu[0] >> 31) & 1
/*
 * TODO: Needs a function to free itself!
 */
void init_mip_cache(uint8_t mip_self, struct mip_cache *cache, int raw_socket)
{
        /* Walk through the interface list */
        struct ifaddrs *ifaces, *ifp;
        struct mip_cache *cache_first, *cache_ptr;
        int interfaces = 0;

        /* Enumerate interfaces: */
        /* Note in man getifaddrs that this function dynamically allocates
           memory. It becomes our responsability to free it! */
        if (getifaddrs(&ifaces)) {
                perror("getifaddrs");
                exit(-1);
        }
        /* The first entry in the cache is reserved for MIP address of the
           *current* node (loopback, kind of)*/
        cache_first = calloc(1, sizeof(struct mip_cache));
        cache_first->mip = mip_self;
        cache_ptr = cache_first;

        /* Walk the list looking for ifaces interesting to us */
        for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
                /* We make certain that the ifa_addr member is actually set */
                if (ifp->ifa_addr == NULL
                    || ifp->ifa_addr->sa_family != AF_PACKET
                    || strcmp("lo", ifp->ifa_name) < 1)
                        continue;

                /* Allocate memory for interface, and copy address */
                cache_ptr = cache_ptr->next =
                    (struct mip_cache *)calloc(1, sizeof(struct mip_cache));
                memcpy(&(cache_ptr->addr),
                       (struct sockaddr_ll *)ifp->ifa_addr,
                       sizeof(struct sockaddr_ll));
                /* Filling with broadcast address, it's guaranteed not to clash 
                   with an actual MIP address. */
                cache_ptr->mip = MIP_BROADCAST;
                cache_ptr->raw_socket = raw_socket;     /* FIXME: This is
                                                           wastefull */
                interfaces++;
        }

        /* Free the interface list */
        freeifaddrs(ifaces);

        if (interfaces) {
                /* Copy over the first node in linked list to argument pointer. 
                   Free temporary data used to create linked list. */
                memcpy(cache, cache_first, sizeof(struct mip_cache));
                free(cache_first);
        }

        return;
}

struct mip_cache *mip_cache(uint8_t mip_addr, struct mip_cache *cache)
{
        while (cache != NULL) {
                if (cache->mip == mip_addr)
                        return cache;
                cache = cache->next;
        }
        return NULL;
}

int send_arp_request(uint8_t mip_addr, struct mip_cache *cache)
{
        int rc;
        uint8_t eth_broadcast[] = ETH_BROADCAST;
        struct ether_frame frame_hdr;
        struct mip_msg *msg;

        /* Allocate space for ARP message */
        msg = calloc(1, MIP_ARP_MSG_SIZE);

        /* Fill in Ethernet header. ARP request is a BROADCAST packet */
        memcpy(frame_hdr.dst_addr, eth_broadcast, ETH_MAC_LEN);
        frame_hdr.eth_proto = htons(ETH_P_MIP);

        /* Modify MIP message header */
        msg->hdr.dst_addr = MIP_BROADCAST;
        msg->hdr.src_addr = cache->mip;  /* *Current* MIP-address */
        msg->hdr.ttl = 1;           /* FIXME: remove hardcoded */
        msg->hdr.sdu_len = MIP_ARP_SDU_SIZE / 4;
        msg->hdr.sdu_type = MIP_T_ARP;

        /* Modify MIP ARP payload. Type, then address */
        msg->sdu[0] = (MIP_ARP_REQ << 31) | (mip_addr << 23);

        for (; cache->next != NULL; cache = cache->next) {
                /* Modify MAC address for each interface */
                memcpy(&frame_hdr.src_addr, cache->next->addr.sll_addr,
                       ETH_MAC_LEN);
                /* Send packet on all interfaces */
                rc = send_raw_packet(cache->next->raw_socket,
                                     (uint8_t *) msg, MIP_ARP_MSG_SIZE,
                                     &frame_hdr, &cache->next->addr);
                if (rc == -1) {
                        perror("send_raw_packet");
                        return rc;
                }
        }

        return rc;
}

int handle_arp_packet(struct mip_msg *msg,
                      struct mip_cache *cache,
                      struct ether_frame *frame_hdr,
                      struct sockaddr_ll *so_addr)
{
        switch (ARP_TYPE) {
        case MIP_ARP_RES:

                return handle_arp_response(msg, cache, so_addr);

        case MIP_ARP_REQ:

                if (msg->hdr.dst_addr != MIP_BROADCAST) {
                        /* Drop MIP ARP packet if address isn't broadcast */
                        return -1;
                }

                if (mip_cache(ARP_ADDR, cache))
                        /* If the requested MIP address is in cache */
                        return send_arp_response(msg, cache,
                                                 frame_hdr, so_addr);
                else
                        return MIP_CACHE_MISS;
        }

        return -1;
}

int handle_arp_response(struct mip_msg *msg,
                        struct mip_cache *cache, struct sockaddr_ll *so_addr)
{

        /* Find the address of the interface where the ARP packet came from
           using the interface index. */
        /* TODO: Factor out this with same from send_arp_response() from above */
        /* <<< */
        while (cache != NULL) {
                if (cache->addr.sll_ifindex == so_addr->sll_ifindex)
                        break;
                cache = cache->next;
        }
        if (cache == NULL) {
                printf("<mip_arp error: interface not in cache>\n");
                return -1;
        }
        /* >>> */

        /* Update MIP cache with address from response */
        cache->mip = ARP_ADDR;

        memcpy(cache->mac, &so_addr->sll_addr, ETH_MAC_LEN);

        return 1;
}

int send_arp_response(struct mip_msg *msg,
                      struct mip_cache *cache,
                      struct ether_frame *frame, struct sockaddr_ll *so_addr)
{
        uint8_t mip_self = cache->mip;  /* *Current* MIP-address */
        struct mip_hdr *hdr = (struct mip_hdr *)&msg->hdr;

        /* Find the address of the interface where the broadcast packet came
           from using the interface index */
        while (cache != NULL) {
                if (cache->addr.sll_ifindex == so_addr->sll_ifindex)
                        break;
                cache = cache->next;
        }

        if (cache == NULL) {
                printf("<mip_arp error: interface not in cache>\n");
                return -1;
        }

        /* Swap MAC addresses of the ether_frame to send back (unicast) the ARP 
           response. Copy source address from cache to ethernet header */
        memcpy(frame->dst_addr, frame->src_addr, ETH_MAC_LEN);
        memcpy(frame->src_addr, cache->addr.sll_addr, ETH_MAC_LEN);
        frame->eth_proto = htons(ETH_P_MIP);

        /* Update MIP cache with MIP-address from ARP message */
        cache->mip = hdr->src_addr;

        /* Modify MIP message header */
        hdr->dst_addr = hdr->src_addr;
        hdr->src_addr = mip_self;
        hdr->ttl = 1;           /* FIXME: remove hardcoded */
        hdr->sdu_len = MIP_ARP_SDU_SIZE / 4;
        hdr->sdu_type = MIP_T_ARP;
        msg->hdr = *hdr;

        /* Modify MIP ARP payload. That is, change only the type */
        msg->sdu[0] = (msg->sdu[0] & ~(1 << 31)) | (MIP_ARP_RES << 31);

        if (send_raw_packet(cache->raw_socket, (uint8_t *) msg,
                            MIP_ARP_MSG_SIZE, frame, &cache->addr) == -1) {
                perror("send_raw_packet");
                return -1;
        }

        return 1;
}
