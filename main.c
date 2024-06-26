﻿#include <string.h>
#include <time.h>

#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <hardware/watchdog.h>
#include <lwip/apps/httpd.h>
#include <pico/cyw43_arch.h>
#include <pico/util/datetime.h>

#include "utils.h"
#include "lwipopts.h"
#include "picow_ntp_client.h"
#include "info.h"
#include "ssi.h"
#include "cgi.h"

#define COMPILED_ON " (" __DATE__ " - " __TIME__ ")"

int main(void)
{
    stdio_init_all();
    watchdog_enable(10000, 1);
    printf("%s %d\n", COMPILED_ON, getFreeHeap());

    // init wifi chip so we can turn the LED on asap :)
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK))
    {
        printf("failed to initialise\n");
        return 1;
    }
    watchdog_update();
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_SETUP);

    printf("Initialising ADC\n");
    adc_init();
    adc_gpio_init(KY_003_GPIO);
    adc_select_input(2);
    watchdog_update();
    printf("initialised\n");

    cyw43_arch_enable_sta_mode();
    printf("Connecting to %s\n", WIFI_SSID);
    watchdog_update();
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        printf("Failed to connect!\n");
        for (size_t i = 0; i < 10; i++)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(15);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(15);
        }
        watchdog_update();
        // return 1;
    }
    watchdog_update();
    u32_t ip_address = cyw43_state.netif[0].ip_addr.addr; // i hope
    printf("Connected at http://%u.%u.%u.%u\n", ip_address & 0xFF, (ip_address >> 8) & 0xFF, (ip_address >> 16) & 0xFF, (ip_address >> 24) & 0xFF);

    daily_info = init_info();
    daily_info->entry_count = 1;

    datetime_t t = {
        .year = 2020,
        .month = 01,
        .day = 01,
        .dotw = 1, // 0 is Sunday
        .hour = 00,
        .min = 00,
        .sec = 00};

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);
    watchdog_update();
    // Start NTP client
    NTP_T *ntp_state = ntp_init();

    if (!ntp_state)
    {
        printf("NTP initialization failed!\n");
        // return 1;
    }

    request_ntp_update(ntp_state);
    watchdog_update();
    rtc_get_datetime(&t);

    // Initialise web server
    httpd_init();
    printf("HTTP server initialised\n");
    watchdog_update();
    // Configure SSI and CGI handler
    ssi_init();
    printf("SSI Handler initialised\n");
    watchdog_update();
    cgi_init();
    printf("CGI Handler initialised\n");
    watchdog_update();

    printf("Starting main loop\n");

    // char val[10] = "";
    const float conversion_factor = 3.3f / (1 << 12);

    bool good = true;
    bool bounce = true;
    bool added_entry = false;
    uint32_t count = 0;

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_DONE);

    watchdog_enable(1000, 1);

    while (good)
    {
        watchdog_update();
        uint16_t result = adc_read();
        // printf("Raw value: 0x%03x, voltage: %f V\n", result, result * conversion_factor);
        // no magnet (under threshold)
        if (result * conversion_factor >= 1.0)
        { // arbitrary threshold, goes from like 1.8v to 0.07v under normal conditions. can go up to 3v though
            // if previous check had magnet
            if (bounce)
            {
                bounce = false;
                // no change
            }
            else
            {
            }
            // magnet in range
        }
        else
        {
            // if previous check didnt have magnet
            if (!bounce)
            {
                bounce = true;
                daily_info->days[daily_info->entry_count - 1]->counter++;
                printf("%d\n", daily_info->days[daily_info->entry_count - 1]->counter);

                // if previous check had magnet and we're currently in magnet
            }
            else
            {
            }
        }
        // can probably sleep in the loop a little bit
        count++;
        // check every 500 loops
        // hopefully we do more than 500 loops a minute..
        if (count > 500)
        {
            count = 0;

            rtc_get_datetime(&t);
            if (t.hour == 13 && t.min == 30)
            {
                if (!added_entry)
                {
                    added_entry = true;
                    if (daily_info->entry_count <= INFO_DEFAULT_ENTRIES - 5)
                    {
                        DayEntry *de = daily_info->days[daily_info->entry_count];
                        printf("Adding a new entry! %d %d %d %d %d\n", daily_info->entry_count, de->year, de->month, de->day);
                        de->year = t.year;
                        de->month = t.month;
                        de->day = t.day;

                        daily_info->entry_count++;
                    }
                }
            }
            else
            {
                added_entry = false;
            }
        }
    }

    return 0;
}
