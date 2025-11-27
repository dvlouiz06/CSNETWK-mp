#include <winsock2.h> // winsock library for winsock2 api functions
#include <ws2tcpip.h>
#include <stdio.h>

// link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define PORT 6000
#define BUFLEN 512


void cleanup(SOCKET s) {//function to clean up Winsock
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    WSACleanup();
}

int main() {
    WSADATA wsaData; //receives details about the Winsock implementation whem calling WSAStartup()
    SOCKET hostSocket = INVALID_SOCKET; //declares a socket variable to hold the UDP server socket
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char recvBuf[BUFLEN];
    int recvLen;
    int result;

    printf("- UDP Host Peer (Server) -\n");

    // 1.initialize Winsock
    // request winsock version 2.2
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);//oon success. wsadata is filled with information about the winsock implementation
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    printf("Winsock initialized.\n");

    // 2.create a socket
    
    hostSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);// creates a UDP soclket. af_inet for ipv4, sock_dgram for UDP
    if (hostSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());// id creation failed returns invalid socket and retrieves the winsock specific error
        cleanup(hostSocket);
        return 1;
    }
    printf("UDP socket created.\n");

    // 3. setup the sockaddr_in structure for binding
    serverAddr.sin_family = AF_INET; //ipv4
    
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);// isten on all interfaces
    //port to listen on network byte order
    serverAddr.sin_port = htons(PORT);

    //4. bind the socket to the address and port
    result = bind(hostSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        cleanup(hostSocket);
        return 1;
    }
    printf("Socket bound to port %d. Waiting for data...\n", PORT);

    // 5. receive a datagram simulating a handshakert
    // The Host Peer waits indefinitely for a message.
    recvLen = recvfrom(hostSocket, // = socket descriptor
                       recvBuf, //  = pointer to buffer where incoming data will be stored
                       BUFLEN, // = max num of bytes to read
                       0, 
                       (struct sockaddr*)&clientAddr, // = pointer to a sockaddr struck that will be filled with the senders Ip and PORT
                       &clientAddrLen);

    if (recvLen == SOCKET_ERROR) {
        printf("recvfrom failed with error: %d\n", WSAGetLastError());
    } else {
        // nullterminate the received data for printing
        if (recvLen < BUFLEN) {
            recvBuf[recvLen] = '\0';
        } else {
            recvBuf[BUFLEN - 1] = '\0'; //
        }

        // convert the clients ip address to a readable string
        char clientIP[INET_ADDRSTRLEN];
        InetNtop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

        printf("\n--- Message Received ---\n");
        printf("From IP: %s, Port: %d\n", clientIP, ntohs(clientAddr.sin_port));
        printf("Message: %s\n", recvBuf);
        printf("------------------------\n");

        // 6. send a response simulating a handshake response
        const char *responseMsg = "HANDSHAKE_RESPONSE\nseed: 12345";
        result = sendto(hostSocket, 
                        responseMsg, 
                        strlen(responseMsg), 
                        0, 
                        (struct sockaddr*)&clientAddr, 
                        clientAddrLen);
        if (result == SOCKET_ERROR) {
            printf("sendto failed with error: %d\n", WSAGetLastError());
        } else {
            printf("Response sent back to client.\n");
        }
    }

    // 7. cleanup
    printf("\nCleaning up and shutting down...\n");
    cleanup(hostSocket);
    return 0;
}