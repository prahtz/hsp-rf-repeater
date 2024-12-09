/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <iostream>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "rc-switch-pico/radio-switch.h"
#include "TCPComm.cpp"


using namespace std;


void receive() {
    const uint RADIO_RECEIVER_PIN = 18;
    gpio_init(RADIO_RECEIVER_PIN);
    
    RCSwitch rcSwitch;
    rcSwitch.enableReceive(RADIO_RECEIVER_PIN);
    gpio_disable_pulls(RADIO_RECEIVER_PIN);
    while (true) {
        if (rcSwitch.available()) {
            unsigned int code = rcSwitch.getReceivedValue();
            std::cout << "VALUE RECEIVED: " << code << std::endl;
            rcSwitch.resetAvailable();
            multicore_fifo_push_blocking(code);
            
        }
        sleep_ms(10);
    }
}

bool wifiInit() {
    if (cyw43_arch_init()) {
        string err = "Wifi Initialization Failed";
        cout << err << endl;
        sleep_ms(1000);
        return false;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        cout << "Connection Failed" << endl;
        return false;
    }
    cout << "Connected" << endl;
    return true;
}

int main() {
    stdio_init_all();
    while (!wifiInit());
    multicore_launch_core1(receive);
    while (true) {
        unsigned int g = multicore_fifo_pop_blocking();
        cout << g << endl;
        TCPComm tcp;
        string message = to_string(g) + "//code//eom";
        tcp.sendMessage(message);
        tcp.close();
    }
}