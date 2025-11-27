#include "reliability.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

RL_Pending rlPending = {0};
uint32_t rlNextSeq = 1;

uint64_t rl_now_ms() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return ul.QuadPart / 10000ULL;
}

void rl_init() {
    rlPending.active = 0;
    rlNextSeq = 1;
}

void rl_send(SOCKET sock, struct sockaddr_in *addr, int addrLen,
             const char *msg)
{
    rlPending.active = 1;
    rlPending.seq = rlNextSeq++;
    rlPending.retries = 0;
    rlPending.lastSend = rl_now_ms();
    rlPending.addr = *addr;
    rlPending.addrLen = addrLen;

    // Build message with sequence
    sprintf(rlPending.data, "%ssequence_number: %u\n", msg, rlPending.seq);
    rlPending.len = (int)strlen(rlPending.data);

    // Log attempt like screenshot
    printf("Sending attempt 1/%d. Seq: %u\n", RL_MAX_RETRIES, rlPending.seq);
    int sent = sendto(sock, rlPending.data, rlPending.len, 0,
                      (struct sockaddr*)&rlPending.addr, rlPending.addrLen);
    if (sent == SOCKET_ERROR) {
        printf("[RL] sendto error: %d\n", WSAGetLastError());
    } else {
        printf("[RL] Sent SEQ=%u\n", rlPending.seq);
    }
}

void rl_handle_ack(const char *msg)
{
    const char *p = strstr(msg, "ack_number:");
    if (!p) return;

    uint32_t ack = (uint32_t)atoi(p + 12);

    if (rlPending.active && rlPending.seq == ack) {
        printf("[RL] ACK received for SEQ=%u\n", ack);
        rlPending.active = 0;
    }
}

void rl_handle_inbound_seq(SOCKET sock,
                           struct sockaddr_in *addr, int addrLen,
                           const char *msg)
{
    const char *p = strstr(msg, "sequence_number:");
    if (!p) return;

    uint32_t seq = (uint32_t)atoi(p + 16);

    char ack[128];
    sprintf(ack,
        "message_type: ACK\n"
        "ack_number: %u\n",
        seq
    );

    sendto(sock, ack, (int)strlen(ack), 0,
           (struct sockaddr*)addr, addrLen);

    printf("[RL] Sent ACK for %u\n", seq);
}

void rl_tick(SOCKET sock)
{
    if (!rlPending.active) return;

    uint64_t now = rl_now_ms();
    if (now - rlPending.lastSend >= RL_TIMEOUT_MS) {

        if (rlPending.retries >= RL_MAX_RETRIES) {
            printf("### Maximum retries reached for sequence %u. Connection assumed lost. ###\n", rlPending.seq);
            printf("Joiner failed to send reliable HANDSHAKE_REQUEST.\n");
            rlPending.active = 0;
            return;
        }

        // Log timeout and retry like screenshot
        printf("Timeout (500ms) reached. No ACK received. Retrying...\n");
        rlPending.retries++;
        printf("Sending attempt %d/%d. Seq: %u\n", rlPending.retries + 1, RL_MAX_RETRIES, rlPending.seq);

        rlPending.lastSend = now;

        int sent = sendto(sock, rlPending.data, rlPending.len, 0,
               (struct sockaddr*)&rlPending.addr, rlPending.addrLen);

        if (sent == SOCKET_ERROR) {
            printf("[RL] sendto error on retransmit: %d\n", WSAGetLastError());
        } else {
            printf("[RL] RETRANSMIT SEQ=%u (try %d)\n", rlPending.seq, rlPending.retries);
        }
    }
}
