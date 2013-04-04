#include "spi.h"
short start_self_check()
{
  short value;
  while(1)
  {
    value = spiSend(CHECK_ADR, 0);
    if(((value & 0x7) == 0x4) || ((value & 0x7) == 0x2))
      return value;
  }
}

void get_check_status()
{
  return spiSend(CHECK_ADR, 0);
}
