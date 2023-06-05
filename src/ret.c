#include "ks/ret.h"
#include <string.h>

const char * ks_ret_name(ks_ret_t ret)
{
  switch (ret)
  {
    #define XX(name, code, message) \
      case KS_ ## name: return message;

    KS__ERR_MAP(XX)
    #undef XX

    default: return "unknown erorr";
  }

  KS_RET_CHECKED(3);
}

char * ks_ret_name_r(ks_ret_t ret, char * p_buffer, size_t buflen)
{
  const char * name = ks_ret_name(ret);

  return strncpy(p_buffer, name, buflen);
}

