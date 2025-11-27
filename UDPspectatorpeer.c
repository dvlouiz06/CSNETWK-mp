#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>


#pragma comment(lib, "Ws2_32.lib")

#define HOST_IP "127.0.0.1" // host peer IP
#define PORT 6000
#define BUFLEN 512

// Function to clean up Winsock
void cleanup(SOCKET s) {
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET spectatorSocket = INVALID_SOCKET;
    struct sockaddr_in hostAddr, fromAddr;
    int fromAddrLen = sizeof(fromAddr);
    char recvBuf[BUFLEN];
    int recvLen;
    int result;

    printf("--- UDP Spectator Peer ---\n");

    // 1.initialize winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    printf("Winsock initialized.\n");

    // 2. create UDP socket
    spectatorSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (spectatorSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        cleanup(spectatorSocket);
        return 1;
    }
    printf("UDP socket created.\n");

    // 3. setup the host address structure
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(PORT);

    // convert Ip string to binary form
    result = InetPton(AF_INET, HOST_IP, &hostAddr.sin_addr);
    if (result != 1) { 
        printf("InetPton failed to convert IP address %s\n", HOST_IP);
        cleanup(spectatorSocket);
        return 1;
    }

    // 4. send the spectator request message
    const char *requestMsg = "message_type: SPECTATOR_REQUEST";
    printf("Sending SPECTATOR_REQUEST to %s:%d...\n", HOST_IP, PORT);
    
    result = sendto(spectatorSocket, 
                    requestMsg, //pointer to the message that will be sent
                    strlen(requestMsg), //specifies the num of bytes to send
                    0, 
                    (struct sockaddr*)&hostAddr, // this is the destination address cast to a sockaddr* 
                    sizeof(hostAddr));

    if (result == SOCKET_ERROR) {
        printf("sendto failed with error: %d\n", WSAGetLastError());
        cleanup(spectatorSocket);
        return 1;
    }
    printf("Request sent successfully.\n");

    // 5.simulate listening for battle updates (like battle setup messages)
    printf("Awaiting first battle update (Host must be running and send a message)...\n");
    // to simulate listenin, we will block and wait for one packet.
    recvLen = recvfrom(spectatorSocket, 
                       recvBuf, // the message
                       BUFLEN, 
                       0, 
                       (struct sockaddr*)&fromAddr, // the senders address
                       &fromAddrLen);

    if (recvLen == SOCKET_ERROR) {
        printf("recvfrom failed with error: %d\n", WSAGetLastError());
    } else {
        if (recvLen < BUFLEN) {
            recvBuf[recvLen] = '\0';//ensures text is null terminated
        } else {
            recvBuf[BUFLEN - 1] = '\0';
        }

        char senderIP[INET_ADDRSTRLEN];
        InetNtop(AF_INET, &(fromAddr.sin_addr), senderIP, INET_ADDRSTRLEN);// convert senders ip back to text

        printf("\n--- Battle Update Received ---\n");
        printf("From IP: %s, Port: %d\n", senderIP, ntohs(fromAddr.sin_port));
        printf("Message: %s\n", recvBuf);
        printf("------------------------------\n");
    }

    // 6.cleanup
    printf("\nCleaning up and shutting down...\n");
    cleanup(spectatorSocket);
    return 0;
}