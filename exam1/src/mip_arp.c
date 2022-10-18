#include "common.h"
#include "mip_arp.h"

#define MIP_ARP_MSG_SIZE sizeof(struct mip_msg) + sizeof(struct mip_arp)

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
           *current* node */
        cache_first = (struct mip_cache *)calloc(1, sizeof(struct mip_cache));
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

int in_mip_cache(uint8_t mip_addr, struct mip_cache *cache)
{
        while (cache != NULL) {
                if (cache->mip == mip_addr)
                        return 1;
                cache = cache->next;
        }
        printf("<info: MIP address not in cache>\n");
        return 0;
}

int send_arp_request(uint8_t mip_addr, struct mip_cache *cache)
{
        /* 
         * TODO: Function is untested!
         */
        int rc;
        uint8_t mip_self = cache->mip;  /* *Current* MIP-address */
        uint8_t dst_addr[] = ETH_BROADCAST;
        struct ether_frame frame_hdr;
        struct mip_msg *msg = (struct mip_msg *)calloc(1, MIP_ARP_MSG_SIZE);
        struct mip_hdr *hdr = &msg->hdr;
        struct mip_arp *arp = (struct mip_arp *)msg->sdu;

        /* Fill in Ethernet header. ARP request is a BROADCAST packet */
        memcpy(frame_hdr.dst_addr, dst_addr, ETH_MAC_LEN);
        frame_hdr.eth_proto = htons(ETH_P_MIP);

        /* Modify MIP message header */
        hdr->dst_addr = MIP_BROADCAST;
        hdr->src_addr = mip_self;
        hdr->ttl = 1;           /* FIXME: remove hardcoded */
        hdr->sdu_len = sizeof(struct mip_arp) / 4;
        hdr->sdu_type = MIP_T_ARP;
        msg->hdr = *hdr;

        /* Modify MIP ARP payload */
        arp->type = MIP_ARP_REQ;
        arp->addr = mip_addr;

        for (; cache->next != NULL; cache = cache->next) {
                /* Modify MAC address for each interface */
                memcpy(&frame_hdr.src_addr, cache->next->addr.sll_addr,
                       ETH_MAC_LEN);
                /* Send packet on all interfaces */
                rc = send_raw_packet(cache->next->raw_socket,
                                     (uint8_t *) msg, MIP_MSG_LEN,
                                     &frame_hdr, &cache->next->addr);
                if (rc == -1) {
                        perror("send_raw_packet");
                }
        }

        free(msg);

        return rc;
}

int handle_arp_packet(struct mip_msg *msg,
                      struct mip_cache *cache,
                      struct ether_frame *frame_hdr,
                      struct sockaddr_ll *so_addr)
{
        struct mip_arp *arp = (struct mip_arp *)msg->sdu;

        /* TODO: Fix debug print */
        printf("<ARP packet from ");
        print_mac_addr(frame_hdr->src_addr, 6);
        printf(" for MIP address %d>\n", (uint8_t) arp->addr);

        switch (arp->type) {
        case MIP_ARP_RES:

                return handle_arp_response(arp, cache, so_addr);

        case MIP_ARP_REQ:

                if (msg->hdr.dst_addr != MIP_BROADCAST) {
                        printf
                            ("<error: MIP header dst.address not broadcast>\n");
                        return -1;
                }

                if (in_mip_cache(arp->addr, cache))
                        /* If the requested MIP address is in cache */
                        return send_arp_response(msg, cache,
                                                 frame_hdr, so_addr);
                else
                        return MIP_CACHE_MISS;  /* TODO: If the requested MIP
                                                   address is *not* in cache */
        }

        return -1;
}

int handle_arp_response(struct mip_arp *arp,
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
                printf("<error: interface not in cache>\n");
                return -1;
        }
        /* >>> */

        /* Update MIP cache with address from response */
        cache->mip = arp->addr;
        memcpy(cache->mac, &so_addr->sll_addr, ETH_MAC_LEN);

        /* TODO: Fix debug print */
        printf("<updated MIP cache: %d --> ", cache->mip);
        print_mac_addr(cache->mac, 6);
        printf(">\n");
        return 1;
}

int send_arp_response(struct mip_msg *msg,
                      struct mip_cache *cache,
                      struct ether_frame *frame,
                      struct sockaddr_ll *so_addr)
{
        uint8_t mip_self = cache->mip;  /* *Current* MIP-address */
        struct mip_hdr *hdr = (struct mip_hdr *)&msg->hdr;
        struct mip_arp *arp = (struct mip_arp *)msg->sdu;

        /* Find the address of the interface where the broadcast packet came
           from using the interface index */
        while (cache != NULL) {
                if (cache->addr.sll_ifindex == so_addr->sll_ifindex)
                        break;
                cache = cache->next;
        }
        if (cache == NULL) {
                printf("<Error: interface not in cache>\n");
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
        hdr->sdu_len = sizeof(struct mip_arp) / 4;
        hdr->sdu_type = MIP_T_ARP;
        msg->hdr = *hdr;

        /* Modify MIP ARP payload */
        arp->type = MIP_ARP_RES;
        arp->addr = arp->addr;  /* Is same */

        if (send_raw_packet(cache->raw_socket,
                            (uint8_t *) msg,
                            sizeof(hdr) + hdr->sdu_len * 4,
                            frame, &cache->addr) == -1) {
                perror("send_raw_packet");
                return -1;
        }

        printf("<responded to ARP from ");
        print_mac_addr(frame->dst_addr, 6);
        printf(">\n");

        return 1;
}
