#ifndef _CGI_H_
#define _CGI_H_

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"

#include "picowota/reboot.h"

#include "info.h"

// CGI handler which is run when a request for /reflash.cgi is detected
const char * cgi_reflash_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    picowota_reboot(true);
    // Send the index page back to the user
    // Not sure if it'll even make it past this though!
    return "/index.shtml";
}

// CGI handler which is run when a request for /reset_counter.cgi is detected
const char * cgi_reset_counter_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    daily_info->current_counter = 0;
    // Send the index page back to the user
    // Not sure if it'll even make it past this though!
    return "/index.shtml";
}


// tCGI Struct
// Fill this with all of the CGI requests and their respective handlers
static const tCGI cgi_handlers[] = {
    {
        // Html request for "/led.cgi" triggers cgi_handler
        {"/reflash.cgi", cgi_reflash_handler},
        {"/reset_counter.cgi", cgi_reset_counter_handler}
    },
};

void cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers, 1);
}

#endif