/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "picow_ntp_client.h"

#include "info.h"

// Called with results of operation
void ntp_result(NTP_T *state, int status, time_t *result)
{
    if (status == 0 && result)
    {
        struct tm *utc = gmtime(result);
        printf("Got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
               utc->tm_hour, utc->tm_min, utc->tm_sec);

        datetime_t t = {
            .year = utc->tm_year + 1900,
            .month = utc->tm_mon + 1,
            .day = utc->tm_mday,
            .dotw = 1, // 0 is Sunday, so 3 is Wednesday
            .hour = utc->tm_hour,
            .min = utc->tm_min,
            .sec = utc->tm_sec};
        printf("Updated the RTC\n");
        // Start the RTC
        rtc_set_datetime(&t);

        // we can update today's entry as well :) cheeky hack
        daily_info->days[daily_info->entry_count - 1]->year = t.year;
        daily_info->days[daily_info->entry_count - 1]->month = t.month;
        daily_info->days[daily_info->entry_count - 1]->day = t.day;

    }

    if (state->ntp_resend_alarm > 0)
    {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }

    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// Make an NTP request
void ntp_request(NTP_T *state)
{
    printf("NTP request in progress\n");

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    printf("NTP request failed\n");
    NTP_T *state = (NTP_T *)user_data;
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    printf("NTP DNS found\n");

    NTP_T *state = (NTP_T *)arg;
    if (ipaddr)
    {
        state->ntp_server_address = *ipaddr;
        printf("NTP address %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    }
    else
    {
        printf("NTP dns request failed\n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    printf("NTP data received\n");

    NTP_T *state = (NTP_T *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0)
    {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    }
    else
    {
        printf("Invalid ntp response\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
NTP_T *ntp_init(void)
{
    printf("NTP Initialisation\n");

    NTP_T *state = (NTP_T *)calloc(1, sizeof(NTP_T));

    if (!state)
    {
        printf("Failed to allocates tate\n");
        return NULL;
    }

    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb)
    {
        printf("Failed to create pcb\n");
        free(state);
        return NULL;
    }

    udp_recv(state->ntp_pcb, ntp_recv, state);

    return state;
}

// Runs ntp test forever
void run_ntp_test(void)
{
    NTP_T *state = ntp_init();
    if (!state)
        return;
    while (true)
    {
        if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 && !state->dns_request_sent)
        {
            // Set alarm in case udp requests are lost
            state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

            // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
            // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
            // these calls are a no-op and can be omitted, but it is a good practice to use them in
            // case you switch the cyw43_arch type later.
            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
            cyw43_arch_lwip_end();

            state->dns_request_sent = true;
            if (err == ERR_OK)
            {
                ntp_request(state); // Cached result
            }
            else if (err != ERR_INPROGRESS)
            { // ERR_INPROGRESS means expect a callback
                printf("DNS request failed\n");
                ntp_result(state, -1, NULL);
            }
        }
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer interrupt) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(state->dns_request_sent ? at_the_end_of_time : state->ntp_test_time);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    free(state);
}

void request_ntp_update(NTP_T *ntp_state)
{
    printf("Requesting NTP update\n");
    if (absolute_time_diff_us(get_absolute_time(), ntp_state->ntp_test_time) < 0 && !ntp_state->dns_request_sent)
    {

        // Set alarm in case udp requests are lost
        ntp_state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, ntp_state, true);

        // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
        // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
        // these calls are a no-op and can be omitted, but it is a good practice to use them in
        // case you switch the cyw43_arch type later.
        cyw43_arch_lwip_begin();

        int err = dns_gethostbyname(NTP_SERVER, &ntp_state->ntp_server_address, ntp_dns_found, ntp_state);
        cyw43_arch_lwip_end();

        ntp_state->dns_request_sent = true;
        if (err == ERR_OK)
        {
            ntp_request(ntp_state); // Cached result
        }
        else if (err != ERR_INPROGRESS)
        { // ERR_INPROGRESS means expect a callback
            printf("DNS request failed\n");
            ntp_result(ntp_state, -1, NULL);
            return false;
        }
    }
}