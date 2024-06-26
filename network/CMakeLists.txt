set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")

aux_source_directory(. DIR_net_SRCS)
add_library(Network ${DIR_net_SRCS})

message("Running makefsdata python script at ${CMAKE_CURRENT_LIST_DIR}")
execute_process(COMMAND
    python makefsdata.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(PICOWOTA_WIFI_SSID ${WIFI_SSID})
set(PICOWOTA_WIFI_PASS ${WIFI_PASSWORD})

target_compile_definitions(Network PUBLIC
WIFI_SSID=\"${WIFI_SSID}\"
WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
PICOWOTA_WIFI_SSID=\"${WIFI_SSID}\"
PICOWOTA_WIFI_PASS=\"${WIFI_PASSWORD}\"
)

target_include_directories(Network PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
)
target_link_libraries(Network PUBLIC
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_stdlib
        hardware_adc
        hardware_rtc
        picowota_reboot
        Info
)
