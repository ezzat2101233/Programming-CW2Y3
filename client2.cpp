#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>  // Include string header for std::string

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Server address configuration
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4002);
    if (inet_pton(AF_INET, "192.168.1.9", &serverAddress.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported \n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Send connection request
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Get message from user
    std::string userInput;
    std::cout << "Enter a message to send to the server: ";
    getline(std::cin, userInput);

    // Send data
    int bytesSent = send(clientSocket, userInput.c_str(), userInput.length(), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Message sent to server: " << userInput << std::endl;
    }

    // Close socket and cleanup
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}