#ifndef _SSI_H_
#define _SSI_H_

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

#include "config.h"

InfoStruct *daily_info;
uint32_t counter = 0;

// SSI tags - tag length limited to 8 bytes by default
const char *ssi_tags[] = {"counter", "dist", "heapinfo", "cinfo"};

u16_t ssi_handler(const char *tag, char *insert, int insertlen,
			u16_t current_tag_part, u16_t *next_tag_part)
{
  printf("SSI: %s, %d, %d/%d\n",tag, insertlen, current_tag_part, next_tag_part);
  size_t printed;
  switch (iIndex)
  {
  case 0: // counter
  {
    printed = snprintf(insert, insertlen, "%d", counter);
  }
  break;
  case 1: // counter distance
  {
    printed = snprintf(insert, insertlen, "%.05fkm", C2KM(counter));
  }
  break;
  case 2: // heap info
  {
    printed = snprintf(insert, insertlen, 
    "%d out of %d", getFreeHeap(), getTotalHeap());
  }
  break;
  case 3:
  {
    // count size
    // i.e 43 + 12 * count + 22 * count
    printf("pc: %d, isl: %d\n", insert, insertlen);

    if (current_tag_part == 0){
      printed = snprintf(insert, insertlen, TABLE_HEADER);
    } else if (current_tag_part == daily_info->entry_count + 1){
      printed = snprintf(insert, insertlen, TABLE_END);
    } else {
      printf("%d/%d\n", current_tag_part, daily_info->entry_count);
      DayEntry *de = daily_info->days[current_tag_part - 1]; 

      printed = snprintf(insert, insertlen,       
      "<tr>"
        "<td>%d/%d/%d</td>"
        "<td>%d</td>"
        "<td>%.05fkm</td>"
      "</tr>", de->year, de->month, de->day, de->counter, C2KM(de->counter));

      next_tag_part = current_tag_part + 1;
    }

);

  }
  break;
  default:
    printed = 0; // 0 bogos binted
    break;
  }

  return (u16_t)printed;
}
// Initialise the SSI handler
void ssi_init()
{

  http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}

#endif