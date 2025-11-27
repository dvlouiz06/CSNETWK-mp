#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1" // Localhost IP for testing
#define PORT 6000
#define BUFLEN 512

//function to clean up winsock
void cleanup(SOCKET s) {
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET joinerSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr, fromAddr;
    int fromAddrLen = sizeof(fromAddr);
    char recvBuf[BUFLEN];
    int recvLen;
    int result;

    printf("--- UDP Joiner Peer (Client) ---\n");

    // 1. initialize Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    printf("Winsock initialized.\n");

    // 2. create a socket
    joinerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (joinerSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        cleanup(joinerSocket);
        return 1;
    }
    printf("UDP socket created.\n");

    // setup the server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT); // Port in Network Byte Order

    // convert ip string to binary form
    result = InetPton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    if (result != 1) { // 1 indicates success
        printf("InetPton failed to convert IP address %s\n", SERVER_IP);
        cleanup(joinerSocket);
        return 1;
    }

    // 4. send a message simul;ates handshaek req
    const char *message = "HANDSHAKE_REQUEST";
    printf("Sending message to %s:%d...\n", SERVER_IP, PORT);
    
    result = sendto(joinerSocket, 
                    message, 
                    strlen(message), 
                    0, 
                    (struct sockaddr*)&serverAddr, 
                    sizeof(serverAddr));

    if (result == SOCKET_ERROR) {
        printf("sendto failed with error: %d\n", WSAGetLastError());
        cleanup(joinerSocket);
        return 1;
    }
    printf("Message sent successfully: '%s'\n", message);

    // 5. receive the response from the server
    printf("Waiting for Host Peer response...\n");
    recvLen = recvfrom(joinerSocket, 
                       recvBuf, 
                       BUFLEN, 
                       0, 
                       (struct sockaddr*)&fromAddr, 
                       &fromAddrLen);

    if (recvLen == SOCKET_ERROR) {
        printf("recvfrom failed with error: %d\n", WSAGetLastError());
    } else {
        // Nulllterminate the received data for printing
        if (recvLen < BUFLEN) {
            recvBuf[recvLen] = '\0';
        } else {
            recvBuf[BUFLEN - 1] = '\0';
        }

        
        char senderIP[INET_ADDRSTRLEN]; // convert the sender ip address to a readable string
        InetNtop(AF_INET, &(fromAddr.sin_addr), senderIP, INET_ADDRSTRLEN);

        printf("\n--- Response Received ---\n");
        printf("From IP: %s, Port: %d\n", senderIP, ntohs(fromAddr.sin_port));
        printf("Response: %s\n", recvBuf);
        printf("-------------------------\n");
    }

    // 6. cleanup
    printf("\nCleaning up and shutting down...\n");
    cleanup(joinerSocket);
    return 0;
}