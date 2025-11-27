// UDPhostpeer.c (auto-attack enabled)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include "reliability.h"
#include "msg.h"
#include "battleflow.h"
#include "rng.h"
#include "damage.h"

// link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define PORT 6000
#define BUFLEN 512

void cleanup(SOCKET s) {
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET hostSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char recvBuf[BUFLEN];
    int recvLen;
    int result;

    printf("- UDP Host Peer (Server) -\n");

    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    printf("Winsock initialized.\n");

    hostSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hostSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        cleanup(hostSocket);
        return 1;
    }
    printf("UDP socket created.\n");

    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    result = bind(hostSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        cleanup(hostSocket);
        return 1;
    }
    printf("Socket bound to port %d. Waiting for data...\n", PORT);

    // init modules
    rl_init();
    BattleState bstate;
    bf_init(&bstate);

    printf("Host Peer running. Waiting for HANDSHAKE_REQUEST on port %d...\n", PORT);

    fd_set readfds;
    struct timeval tv;
    int selectRes;
    int running = 1;

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(hostSocket, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 50000;

        selectRes = select(0, &readfds, NULL, NULL, &tv);
        if (selectRes == SOCKET_ERROR) {
            printf("select failed with error: %d\n", WSAGetLastError());
            break;
        }

        if (selectRes > 0 && FD_ISSET(hostSocket, &readfds)) {
            recvLen = recvfrom(hostSocket,
                               recvBuf,
                               BUFLEN,
                               0,
                               (struct sockaddr*)&clientAddr,
                               &clientAddrLen);

            if (recvLen == SOCKET_ERROR) {
                printf("recvfrom failed with error: %d\n", WSAGetLastError());
            } else if (recvLen > 0) {
                if (recvLen < BUFLEN) recvBuf[recvLen] = '\0';
                else recvBuf[BUFLEN - 1] = '\0';

                char clientIP[INET_ADDRSTRLEN];
                InetNtop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

                printf("\n--- Message Received ---\n");
                printf("From IP: %s, Port: %d\n", clientIP, ntohs(clientAddr.sin_port));
                printf("Message: %s\n", recvBuf);
                printf("------------------------\n");

                // reliability hooks
                rl_handle_ack(recvBuf);
                rl_handle_inbound_seq(hostSocket, &clientAddr, clientAddrLen, recvBuf);

                // handshake
                if (strstr(recvBuf, "HANDSHAKE_REQUEST")) {
                    rl_send(hostSocket, &clientAddr, clientAddrLen,
                        "message_type: HANDSHAKE_RESPONSE\nseed: 12345\n");
                    printf("[HOST] Sent reliable HANDSHAKE_RESPONSE (seed:12345)\n");
                    continue;
                }

                // pass to battleflow
                BattleMessage m = msg_parse(recvBuf);
                bf_process_host(&bstate, hostSocket, &clientAddr, clientAddrLen, &m);

                // AUTO-LOOP: host auto-attacks when it's the host's turn
                if (bstate.state == GS_WAITING_FOR_MOVE && bstate.hostTurn == 1) {
                    if (!bstate.attack_pending) {
                        rl_send(hostSocket, &clientAddr, clientAddrLen,
                                "message_type: ATTACK_ANNOUNCE\nmove: Tackle\n");
                        printf("[HOST] Auto-sent ATTACK_ANNOUNCE\n");
                        bstate.attack_pending = 1;
                    }
                } else {
                    bstate.attack_pending = 0;
                }
            }
        }

        rl_tick(hostSocket);

        if (bstate.state == GS_GAME_OVER) {
            printf("[HOST] Battle ended, exiting loop.\n");
            running = 0;
        }

        Sleep(5);
    }

    printf("\nCleaning up and shutting down...\n");
    cleanup(hostSocket);
    return 0;
}
