#ifndef _SSI_H_
#define _SSI_H_
// server side includes!

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

#include "info.h"
#include "config.h"

// SSI tags - tag length limited to 8 bytes by default
const char *ssi_tags[] = {"counter", "dist", "heapinfo", "cinfo"};

u16_t ssi_handler(int tag_index, char *insert, int max_insert_len,
			u16_t current_tag_part, u16_t *next_tag_part)
{
  // tag, ssi->tag_insert, current_tag_part, &ssi->tag_part
  printf("SSI: %d, %d, %d/%d\n",tag_index, max_insert_len, current_tag_part, *next_tag_part);
  size_t printed;
  switch (tag_index)
  {
  case 0: // counter
  {
    printed = snprintf(insert, max_insert_len, "%d", daily_info->current_counter);
  }
  break;
  case 1: // counter distance
  {
    printed = snprintf(insert, max_insert_len, "%.05fkm", C2KM(daily_info->current_counter));
  }
  break;
  case 2: // heap info
  {
    printed = snprintf(insert, max_insert_len, 
    "%d out of %d", getFreeHeap(), getTotalHeap());
  }
  break;
  case 3:
  {    
    // first tag part
    if (current_tag_part == 0){
      printed = snprintf(insert, max_insert_len, TABLE_HEADER);
      *next_tag_part = 1;
    // last tag part
    } else if (current_tag_part == daily_info->entry_count + 1){
      printed = snprintf(insert, max_insert_len, TABLE_END);
    } else {
      printf("%d/%d\n", current_tag_part, daily_info->entry_count);
      DayEntry *de = daily_info->days[current_tag_part - 1]; 

      printed = snprintf(insert, max_insert_len,       
      "<tr>\n"
        "<td>%d/%d/%d</td>\n"
        "<td>%d</td>\n"
        "<td>%.05fkm</td>\n"
      "</tr>\n", de->year, de->month, de->day, de->counter, C2KM(de->counter));

      next_tag_part = current_tag_part + (u16_t)1;
    }
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