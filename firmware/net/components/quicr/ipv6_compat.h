/*
 * Compatibility Header: This file provides stub definitions for IPv6 socket options
 * required by the picoquic library to compile on ESP-IDF.
 *
 * WARNING: These definitions do not provide actual IPv6 functionality. Attempting to
 * use these socket options at runtime may result in socket errors, silent failures,
 * or undefined behaviour. Use with extreme caution.
 */

#pragma once

#include <sys/socket.h>

#ifndef IPV6_PKTINFO
#define IPV6_PKTINFO 61
struct in6_pktinfo
{
    struct in6_addr ipi6_addr;
    unsigned int ipi6_ifindex;
};

#endif

#ifndef IPV6_TCLASS
#define IPV6_TCLASS 67
#endif

#ifdef LWIP_IPV4
#define ipi_spec_dst ipi_addr
#endif