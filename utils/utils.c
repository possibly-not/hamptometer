#include "utils.h"

// https://forums.raspberrypi.com/viewtopic.php?t=347638
uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}
// https://forums.raspberrypi.com/viewtopic.php?t=347638
uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}
