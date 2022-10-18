/*
 * File: ping.h
 *
 * Check https://valecs.gitlab.io/resources/CHeaderFileGuidelines.pdf for some
 * more nice practises.
 */

#ifndef _PING_H
#define _PING_H

#include <stdint.h>    /* uint8_t */

struct mip_msg *construct_mip_ping(uint8_t dst_addr, char *buf);
int client(char* dst_host, char *buf, char *socket_name);

#endif /* _PING_H */
