#ifndef RELIABILITY_H
#define RELIABILITY_H

#include <winsock2.h>
#include <stdint.h>

#define RL_MAX_RETRIES 3
#define RL_TIMEOUT_MS 500
#define RL_MAX_DATA 2048

typedef struct {
    int active;
    uint32_t seq;
    uint64_t lastSend;
    int retries;

    char data[RL_MAX_DATA];
    int len;

    struct sockaddr_in addr;
    int addrLen;
} RL_Pending;

extern RL_Pending rlPending;
extern uint32_t rlNextSeq;

void rl_init();
uint64_t rl_now_ms();

void rl_send(SOCKET sock, struct sockaddr_in *addr, int addrLen,
             const char *msg);

void rl_handle_ack(const char *msg);
void rl_handle_inbound_seq(SOCKET sock, struct sockaddr_in *addr,
                           int addrLen, const char *msg);

void rl_tick(SOCKET sock);

#endif
