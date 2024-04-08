#ifndef _SSI_H_
#define _SSI_H_

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

#include "config.h"

uint32_t counter = 0;

// SSI tags - tag length limited to 8 bytes by default
const char *ssi_tags[] = {"counter", "dist", "heapinfo"};

u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
  size_t printed;
  switch (iIndex)
  {
  case 0: // counter
  {
    printed = snprintf(pcInsert, iInsertLen, "%d", counter);
  }
  break;
  case 1: // counter distance
  {
    printed = snprintf(pcInsert, iInsertLen, "%.05fkm", (counter * WHEEL_SIZE_CM * PI) * 0.00001);
  }
  break;
  case 2: // heap info
  {
    printed = snprintf(pcInsert, iInsertLen, "%d out of %d", getFreeHeap(), getTotalHeap());
  }
  break;
  default:
    printed = 0; // bogos binted
    break;
  }

  return (u16_t)printed;
}
// Initialise the SSI handler
void ssi_init(uint32_t *counter_ref)
{
  // setup links (sorry global vars)
  counter = counter_ref;

  http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}

#endif