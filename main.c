﻿#include <string.h>
#include <time.h>

#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <lwip/apps/httpd.h>
#include <pico/cyw43_arch.h>
 
#include "utils.h"
#include "lwipopts.h"
#include "info.c"
#include "ssi.h"
#include "cgi.h"

#define COMPILED_ON " (" __DATE__ " - " __TIME__ ")"

int main(void)
{
    stdio_init_all();
    
    printf("%s %d\n", COMPILED_ON, getFreeHeap());

    // init wifi chip so we can turn the LED on asap :)
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_SETUP);

    printf("Initialising ADC\n");
    adc_init();
    adc_gpio_init(KY_003_GPIO);
    adc_select_input(2);

    printf("initialised\n");

    cyw43_arch_enable_sta_mode();
    printf("Connecting to %s\n", WIFI_SSID);

    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect!\n");
        for (size_t i = 0; i < 10; i++)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(15);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(15);
        }
        
        //return 1;
    }
    u32_t ip_address = cyw43_state.netif[0].ip_addr.addr; // i hope
    printf("Connected at %u.%u.%u.%u\n", ip_address & 0xFF, (ip_address >> 8) & 0xFF, (ip_address >> 16) & 0xFF, (ip_address >> 24) & 0xFF);
    
    // Initialise web server
    httpd_init();
    printf("HTTP server initialised\n");

    // Configure SSI and CGI handler
    ssi_init(counter); 
    printf("SSI Handler initialised\n");
    cgi_init();
    printf("CGI Handler initialised\n");


    printf("Starting main loop\n");

    // char val[10] = "";
    const float conversion_factor = 3.3f / (1 << 12);

    bool good = true;
    bool bounce = true;


    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_DONE);
    while (good)
    {
        uint16_t result = adc_read();
        // printf("Raw value: 0x%03x, voltage: %f V\n", result, result * conversion_factor);        
        // no magnet (under threshold)
        if (result * conversion_factor >= 1.0) {// arbitrary threshold, goes from like 1.8v to 0.07v under normal conditions. can go up to 3v though
            // if previous check had magnet
            if (bounce){
                bounce = false;
            // no change
            } else {
                
            }
        // magnet in range
        } else {
            // if previous check didnt have magnet
            if (!bounce){
                bounce = true;
                counter += 1;

                printf("%d\n", counter);

            // if previous check had magnet and we're currently in magnet
            } else {

            }

        }
        // can probably sleep in the loop a little bit
    }

    return 0;
}


