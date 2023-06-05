#include <stdlib.h>

#include "expect.h"
#include "ks.h"

int main(void)
{
  EXPECT(ks_ipv4_addr_is_valid(
    &(ks_ipv4_addr_t)
    {
      .ip   = KS_IPv4(127, 0, 0, 1),
      .port = 8080,
    }));

  EXPECT(ks_ipv4_addr_is_valid(
    &(ks_ipv4_addr_t)
    {
      .ip   = KS_IPv4(0, 0, 0, 0),
      .port = 80,
    }));

  EXPECT(!ks_ipv4_addr_is_valid(&(ks_ipv4_addr_t) {0}));

  EXPECT(!ks_ipv4_addr_is_valid(
    &(ks_ipv4_addr_t)
    {
      .ip   = KS_IPv4(127, 0, 0, 1),
      .port = 0,
    }));
}
