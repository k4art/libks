#include "ks/ipv4.h"
#include <assert.h>
#include <string.h>

bool ks_ipv4_addr_is_valid(const ks_ipv4_addr_t * ipv4)
{
  return ipv4 && ipv4->port != 0;
}

static void ks__ipv4_ip_to_network(const uint8_t (* ip)[4],
                                   uint32_t * p_u32)
{
  assert(ip);
  assert(p_u32);
  
  uint32_t u32 = 0;

  u32 |= (*ip)[0] << 0;
  u32 |= (*ip)[1] << 8;
  u32 |= (*ip)[2] << 16;
  u32 |= (*ip)[3] << 24;

  *p_u32 = u32;
}

void ks_ipv4_to_os_sockaddr(const ks_ipv4_addr_t * ipv4,
                            struct sockaddr_in   * p_sockaddr)
{
  assert(ipv4);
  assert(p_sockaddr);

  memset(p_sockaddr, 0, sizeof(*p_sockaddr));
  
  p_sockaddr->sin_family = AF_INET;
  p_sockaddr->sin_port   = htons(ipv4->port);

  ks__ipv4_ip_to_network(&ipv4->ip, &p_sockaddr->sin_addr.s_addr);
}
