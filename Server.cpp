#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <map>  // Include map for client tracking

#pragma comment(lib, "ws2_32.lib")

std::map<int, SOCKET> clients;  // Map to store client IDs and sockets
int clientCounter = 0;  // Counter to assign unique client IDs

void handleClient(int clientId, SOCKET clientSocket) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cerr << "Client " << clientId << " disconnected.\n";
            clients.erase(clientId);  // Remove client from map
            break;
        }

        buffer[bytesReceived] = '\0';  // Null-terminate the string
        std::cout << "Client " << clientId << " sent: " << buffer << std::endl;

        // Echo message back to client
        send(clientSocket, buffer, bytesReceived, 0);
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    // Create socket for listening
    SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4002);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(listeningSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    // Listen on the socket
    if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening...\n";

    // Accept incoming connections
    while (true) {
        sockaddr_in client;
        int clientSize = sizeof(client);

        SOCKET clientSocket = accept(listeningSocket, (struct sockaddr*)&client, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Assign unique ID to client
        int clientId = ++clientCounter;
        clients[clientId] = clientSocket;

        // Display client info
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];
        ZeroMemory(host, NI_MAXHOST);
        ZeroMemory(service, NI_MAXSERV);

        if (getnameinfo((struct sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
            std::cout << "Client " << clientId << " connected from " << host << " on port " << service << std::endl;
        }
        else {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            std::cout << "Client " << clientId << " connected from " << host << " on port " << ntohs(client.sin_port) << std::endl;
        }

        // Handle client in a separate thread
        std::thread clientThread(handleClient, clientId, clientSocket);
        clientThread.detach();  // Detach the thread to handle independently
    }

    // Close listening socket
    closesocket(listeningSocket);
    WSACleanup();

    return 0;
}
