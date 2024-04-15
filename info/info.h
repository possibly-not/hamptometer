#ifndef _INFO_H_
#define _INFO_H_

#include <pico/stdlib.h>

#define INFO_DEFAULT_ENTRIES 365
#define TABLE_HEADER "<table>\n<tr><th>Date</th><th>Spins</t<th>Km</th></tr>\n"
#define TABLE_END "</table>\n"
#define C2KM(counter) ((counter * WHEEL_SIZE_CM * PI) * 0.00001)

typedef struct DayEntry_{
    uint32_t year;
    uint8_t month;
    uint8_t day;
    uint32_t counter;
} DayEntry;

typedef struct InfoStruct_{
    uint32_t current_counter;
    uint32_t entry_size;
    uint32_t entry_count;
    DayEntry* days[INFO_DEFAULT_ENTRIES];
} InfoStruct;

// I love global variables
extern InfoStruct *daily_info;

InfoStruct* init_info(void);

#endif