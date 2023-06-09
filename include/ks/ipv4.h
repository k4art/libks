#include <netinet/in.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
  uint8_t  ip[4];
  uint16_t port;
} ks_ipv4_addr_t;

#define KS_IPv4(o0, o1, o2, o3) { o0, o1, o2, o3 }

bool ks_ipv4_addr_is_valid(const ks_ipv4_addr_t * ipv4);

void ks_ipv4_to_os_sockaddr(const ks_ipv4_addr_t * ipv4,
                            struct sockaddr_in   * p_sockaddr);

