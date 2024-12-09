#include <string.h>
#include <time.h>
#include <string>
#include <iostream>
#include <atomic>

using namespace std;

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define BUF_SIZE 2048
#define TCP_PORT 33470

class TCPState {
    public:
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    atomic<bool> connected;
    atomic<bool> complete;
    atomic<bool> ack;
    TCPState() {
        connected = false;
        complete = false;
        ack = true;
    }
};

static void tcp_client_err(void *arg, err_t err) {
    TCPState* state = (TCPState*)arg;
    if (err != ERR_ABRT) {
        printf("tcp_client_err %d\n", err);
        state->complete = true;
    }
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCPState* state = (TCPState*)arg;
    state->ack = true;
    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCPState *state = (TCPState*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        state->complete = true;
        return err;
    }
    state->connected = true;
    return ERR_OK;
}

class TCPComm {
    private:
    TCPState state;
    public:
    TCPComm() {
        ip4addr_aton(TCP_SERVER_IP, &state.remote_addr);
        open();
    }

    void close() {
        cyw43_arch_lwip_begin();
        if (state.tcp_pcb != NULL) {
            tcp_arg(state.tcp_pcb, NULL);
            tcp_sent(state.tcp_pcb, NULL);
            err_t err = tcp_close(state.tcp_pcb);
            if (err != ERR_OK) {
                printf("close failed %d, calling abort\n", err);
                tcp_abort(state.tcp_pcb);
            }
            state.tcp_pcb = NULL;
        }
        cyw43_arch_lwip_end();
    }
    
    bool open() {
        state.tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state.remote_addr));
        if (!state.tcp_pcb) {
            cout << "Failed to create pcb\n";
            return false;
        }

        tcp_arg(state.tcp_pcb, &state);
        //tcp_poll(state->tcp_pcb, tcp_client_poll, 10);
        tcp_sent(state.tcp_pcb, tcp_client_sent);
        //tcp_recv(state.tcp_pcb, tcp_client_recv);
        tcp_err(state.tcp_pcb, tcp_client_err);

        cyw43_arch_lwip_begin();
        err_t err = tcp_connect(state.tcp_pcb, &state.remote_addr, TCP_PORT, tcp_client_connected);
        cyw43_arch_lwip_end();

        while (!state.connected && !state.complete) { //TODO: wait until
            sleep_ms(100);
        }
        return err == ERR_OK;
    }

    bool sendMessage(string msg) {
        while (!state.ack) { //wait until
            sleep_ms(100);
        }
        state.ack = false;
        bool result = true;
        char buffer[msg.length()];
        printf("Writing %ld bytes to server\n", msg.length());
        for(int i=0; i < msg.length(); i++) {
            buffer[i] = msg[i];
        }
        cyw43_arch_lwip_begin();
        err_t err = tcp_write(state.tcp_pcb, &buffer, msg.length(), TCP_WRITE_FLAG_COPY);
        err = tcp_output(state.tcp_pcb);
        cyw43_arch_lwip_end();

        if (err != ERR_OK) {
            printf("Failed to write data %d\n", err);
            state.complete = true;
            result = false;
        }
        return result;
    }

    ~TCPComm() {
        close();
    }
    
};
