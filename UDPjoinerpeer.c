// UDPjoinerpeer.c (auto-loop enabled)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "reliability.h"
#include "msg.h"
#include "battleflow.h"
#include "rng.h"
#include "damage.h"

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 6000
#define BUFLEN 512

void cleanup(SOCKET s) {
    if (s != INVALID_SOCKET) closesocket(s);
    WSACleanup();
}

int main(int argc, char **argv) {
    WSADATA wsaData;
    SOCKET joinerSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr, fromAddr;
    int fromAddrLen = sizeof(fromAddr);
    char recvBuf[BUFLEN];
    int recvLen;
    int result;

    printf("--- UDP Joiner Peer (Client) ---\n");

    result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    printf("Winsock initialized.\n");

    joinerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (joinerSocket == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        cleanup(joinerSocket);
        return 1;
    }
    printf("UDP socket created.\n");

    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    InetPton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // init modules
    rl_init();
    BattleState bstate;
    bf_init(&bstate);

    // send handshake reliably
    rl_send(joinerSocket, &serverAddr, sizeof(serverAddr),
            "message_type: HANDSHAKE_REQUEST\n");
    printf("Sent reliable HANDSHAKE_REQUEST to %s:%d\n", SERVER_IP, PORT);

    // loop
    fd_set readfds;
    struct timeval tv;
    int selectRes;
    int running = 1;

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(joinerSocket, &readfds);

        tv.tv_sec = 0; tv.tv_usec = 50000; // 50ms

        selectRes = select(0, &readfds, NULL, NULL, &tv);
        if (selectRes == SOCKET_ERROR) {
            printf("select failed: %d\n", WSAGetLastError());
            break;
        }

        if (selectRes > 0 && FD_ISSET(joinerSocket, &readfds)) {
            recvLen = recvfrom(joinerSocket, recvBuf, BUFLEN, 0,
                               (struct sockaddr*)&fromAddr, &fromAddrLen);
            if (recvLen == SOCKET_ERROR) {
                printf("recvfrom failed: %d\n", WSAGetLastError());
            } else if (recvLen > 0) {
                if (recvLen < BUFLEN) recvBuf[recvLen] = '\0';
                else recvBuf[BUFLEN-1] = '\0';

                char senderIP[INET_ADDRSTRLEN];
                InetNtop(AF_INET, &(fromAddr.sin_addr), senderIP, INET_ADDRSTRLEN);

                printf("\n--- Packet Received ---\nFrom IP: %s, Port: %d\nPayload: %s\n-----------------------\n",
                       senderIP, ntohs(fromAddr.sin_port), recvBuf);

                // reliability hooks
                rl_handle_ack(recvBuf);
                rl_handle_inbound_seq(joinerSocket, &fromAddr, fromAddrLen, recvBuf);

                // === Robust HANDSHAKE_RESPONSE handler (force-send BATTLE_SETUP) ===
                if (strstr(recvBuf, "HANDSHAKE_RESPONSE") || strstr(recvBuf, "HANDSHAKE_RESPONSE\n") ) {
                    // log raw packet for debugging
                    printf("[JOINER] DEBUG: Received HANDSHAKE_RESPONSE raw payload.\n");

                    // Try to parse seed robustly (allow spaces/newlines)
                    uint32_t seed = 0;
                    char *s = strstr(recvBuf, "seed:");
                    if (s) {
                        s += 5;
                        while (*s == ' ' || *s == '\t') ++s;
                        seed = (uint32_t)atoi(s);
                        rng_init(seed);
                        printf("[JOINER] Seed set to %u\n", seed);
                    } else {
                        rng_init(12345);
                        printf("[JOINER] No seed found — using fallback seed 12345\n");
                    }

                    // Force-send BATTLE_SETUP reliably
                    printf("[JOINER] About to send BATTLE_SETUP (forced)\n");
                    rl_send(joinerSocket, &fromAddr, fromAddrLen,
                        "message_type: BATTLE_SETUP\nhp: 200\n");
                    printf("[JOINER] Sent BATTLE_SETUP\n");

                    // tiny delay to give the host a moment to receive & ACK
                    Sleep(50);

                    // skip battleflow processing for this packet
                    continue;
                }

                // pass to battleflow
                BattleMessage m = msg_parse(recvBuf);
                bf_process_joiner(&bstate, joinerSocket, &fromAddr, fromAddrLen, &m);

                // AUTO-LOOP: joiner auto-attacks when it's the joiner's turn
                if (bstate.state == GS_WAITING_FOR_MOVE && bstate.hostTurn == 0) {
                    if (!bstate.attack_pending) {
                        rl_send(joinerSocket, &fromAddr, fromAddrLen,
                                "message_type: ATTACK_ANNOUNCE\nmove: Tackle\n");
                        printf("[JOINER] Auto-sent ATTACK_ANNOUNCE\n");
                        bstate.attack_pending = 1;
                    }
                } else {
                    bstate.attack_pending = 0;
                }
            }
        }

        rl_tick(joinerSocket);

        if (bstate.state == GS_GAME_OVER) {
            printf("[JOINER] Battle ended.\n");
            running = 0;
        }

        Sleep(5);
    }

    printf("\nCleaning up and shutting down...\n");
    cleanup(joinerSocket);
    return 0;
}
