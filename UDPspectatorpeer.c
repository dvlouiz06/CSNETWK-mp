#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

#include "reliability.h"
#include "msg.h"
#include "battleflow.h"

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 6000
#define BUFLEN 512

void cleanup(SOCKET s) {
    if (s != INVALID_SOCKET) closesocket(s);
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET specSock = INVALID_SOCKET;
    struct sockaddr_in serverAddr, fromAddr;
    int fromLen = sizeof(fromAddr);
    char buf[BUFLEN];

    printf("--- UDP Spectator ---\n");
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return 1;
    specSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    InetPton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // Send discovery or just bind and listen (simple listen)
    bind(specSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    rl_init();
    BattleState b; bf_init(&b);

    while (1) {
        int r = recvfrom(specSock, buf, BUFLEN, 0, (struct sockaddr*)&fromAddr, &fromLen);
        if (r > 0) {
            buf[r < BUFLEN ? r : BUFLEN-1] = '\0';
            printf("[SPEC] %s\n", buf);
            rl_handle_inbound_seq(specSock, &fromAddr, fromLen, buf);
        }
        rl_tick(specSock);
        Sleep(50);
    }

    cleanup(specSock);
    return 0;
}
