#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4002);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::string choice;
    std::cout << "Welcome to the Chat App!\n";
    std::cout << "1. Sign Up\n";
    std::cout << "2. Log In\n";
    std::cout << "Choose an option (1 or 2): ";
    getline(std::cin, choice);

    std::string command;
    std::string username, password;

    if (choice == "1") {
        std::cout << "Enter a username: ";
        getline(std::cin, username);
        std::cout << "Enter a password: ";
        getline(std::cin, password);
        command = "SIGNUP " + username + " " + password;
    }
    else if (choice == "2") {
        std::cout << "Enter your username: ";
        getline(std::cin, username);
        std::cout << "Enter your password: ";
        getline(std::cin, password);
        command = "LOGIN " + username + " " + password;
    }
    else {
        std::cout << "Invalid choice.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    send(clientSocket, command.c_str(), command.length(), 0);

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, 1024, 0);
    if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
        std::cerr << "Server disconnected.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    buffer[bytesReceived] = '\0';
    if (strcmp(buffer, "SIGNUP_SUCCESS") == 0 || strcmp(buffer, "LOGIN_SUCCESS") == 0) {
        std::cout << "Successfully logged in!\n";
    }
    else if (strcmp(buffer, "CREDENTIALS_EXISTS") == 0) {
        std::cout << "Credentials already exist. Please sign up with new ones.\n";
    }
    else if (strcmp(buffer, "ACCESS_DENIED") == 0) {
        std::cout << "Access denied. Please enter valid credentials.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Now handle chat messages
    while (true) {
        std::string userInput;
        std::cout << "Enter a message to send to the server (type 'exit' to quit): ";
        getline(std::cin, userInput);

        if (userInput == "exit") {
            break;
        }

        int bytesSent = send(clientSocket, userInput.c_str(), userInput.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cerr << "Server disconnected.\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Server echoed: " << buffer << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
