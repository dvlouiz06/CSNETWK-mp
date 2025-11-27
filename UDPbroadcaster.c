#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>


#pragma comment(lib, "Ws2_32.lib")//link with Ws2_32.lib

#define BROADCAST_PORT 6000
// standard broadcast address for a local network.
// this sends the packet to all devices on the current network segment.
#define BROADCAST_IP "255.255.255.255"


void cleanup(SOCKET s) {// function to clean up Winsock
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET broadcastSocket = INVALID_SOCKET;
    struct sockaddr_in broadcastAddr;
    int result;
    char *message = "message_type: BROADCAST\nhost_name: PokeHost-123-Dunkitjonathan\nport: 6000";
    
    printf("--- UDP Broadcaster (Game Announcer) ---\n");

    // 1.initialize Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    printf("Winsock initialized.\n");

    // 2.create a UDP socket
    broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (broadcastSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        cleanup(broadcastSocket);
        return 1;
    }
    printf("UDP socket created.\n");

    // 3eEnable the broadcast option on the socket
    char broadcast = 1;
    result = setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    if (result == SOCKET_ERROR) {
        printf("setsockopt (SO_BROADCAST) failed with error: %d\n", WSAGetLastError());
        cleanup(broadcastSocket);
        return 1;
    }
    printf("Broadcast option enabled.\n");

    // 4 setup the destination address for the broadcast
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(BROADCAST_PORT);
    // set the ip to the universal broadcast address
    InetPton(AF_INET, BROADCAST_IP, &broadcastAddr.sin_addr.s_addr);

    // 5. send the broadcast message
    printf("Broadcasting game announcement to %s:%d...\n", BROADCAST_IP, BROADCAST_PORT);
    
    result = sendto(broadcastSocket, 
                    message, 
                    strlen(message), 
                    0, 
                    (struct sockaddr*)&broadcastAddr, 
                    sizeof(broadcastAddr));

    if (result == SOCKET_ERROR) {
        printf("sendto failed with error: %d\n", WSAGetLastError());
    } else {
        printf("Broadcast message sent successfully (%d bytes).\n", result);
        printf("Content:\n%s\n", message);
    }

    // 6 . cleanup
    printf("\nCleaning up and shutting down...\n");
    cleanup(broadcastSocket);
    return 0;
}