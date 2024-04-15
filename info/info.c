#include "info.h"

InfoStruct *daily_info;

InfoStruct* init_info(){
    InfoStruct *is = calloc(1, sizeof(InfoStruct));

    for (size_t i = 0; i < INFO_DEFAULT_ENTRIES; i++)
    {
        DayEntry *de = calloc(1, sizeof(DayEntry));
        is->days[i] = de;
    }
    return is;
}
