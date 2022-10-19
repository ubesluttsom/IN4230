---
title: "`IN4230`: Home exam 1"
date: "Autumn 2022"
author: "15605"
---

> A discussion of how MIP is different than IPv4, and how its performance
> compares to IPv4 (one to two paragraphs).

MIP is *much* smaller than IPv4. Both in memory size and address space. MIP has
only 255 unique addresses, while IPv4 has $2^{32}$. The MIP header is only 4
bytes, while IPv4 is *at least* 20 bytes. This makes MIP light weight and fast,
but lacking in options, and not suitable for anything large scale. MIP can only
support $2^3$ different SDU types, while IPv4 supports 255 different protocols.

The addressing in IPv4 is hierarchical, with the individual bits having some
meaning. They are also managed by IANA and other regional authorities. MIP
addresses are self-assigned 

> Discuss why we need to handle the MIP-ARP protocol as a special case. Discuss
> alternative design choices that would allow for a cleaner implementation (two
> to three paragraphs).

We need to handle MIP-ARP differently because it needs access to MAC addresses.
Ideally, MIP would abstract away all data link layer details, such as Ethernet.

Another option could be to have MIP-ARP directly over Ethernet, without
encapsulating it in a MIP header. This is what regular ARP does. IPv4 has its
EtherType (`0x0800`) and ARP has its own (`0x0806`). We could have done
something similar, and it would be more modular. The real ARP (not MIP-ARP) is
protocol agnostic. This would warrant some changes in the MIP-ARP, though,
since we need at least the source address too unicast responses (this is
currently in the MIP header, not the ARP SDU).

> A flow chart summarizing the flow of execution on both the client and server
> side.

```
      PING CLIENT                      PING SERVER
       │                                │
       │                                │
       ▼           -1                   ▼           -1
      Connect sock───────┐             Connect sock───────┐
       │                 │              │                 │
       │                 │              │                 │
       ▼        -1       │              ▼                 │
      Send ping─────────►│       ┌────►Wait               │
       │                 │       │      │                 │
       │                 │       │      │                 │
       ▼                 │       │      ▼             -1  │
      Start timer        │       │◄────Recvived data?─────┤
       │                 │       │  No  │                 │
       │                 │       │      │Yes              │
       ▼                 │       │      ▼            -1   │
┌────►Wait               │       │     Is data ping?──────┤
│      │                 │       │      │                 │
│      │                 │       │      │Yes              │
│      ▼       Yes       │       │      ▼                 ▼
│     Timeout?──────────►│       └─────Send pong──────►Exit failure
│      │                 │
│      │No               │
│      ▼              -1 │
└─────Recvieved data?───►│
  No   │                 │
       │Yes              │
       ▼                 ▼
      Is data a pong?───►Exit failure
       │
       │Yes
       ▼
      Print time and pong             
       │
       │
       ▼                              
      Exit success
```
