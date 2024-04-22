#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <map>
#include <fstream>
#include <sstream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

std::map<int, SOCKET> clients;
std::map<int, std::string> clientNames; // Map to store client names
int clientCounter = 0;

const std::string AUTH_FILE = "Auth.txt";

std::string caesarCipher(const std::string& text, int shift) {
    std::string result = text;
    for (char& c : result) {
        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            c = static_cast<char>((c - base + shift) % 26 + base);
        }
    }
    return result;
}

std::string caesarDecipher(const std::string& text, int shift) {
    return caesarCipher(text, 26 - shift); // Deciphering is just shifting back by 26 - shift
}

bool checkCredentials(const std::string& username, const std::string& password) {
    std::ifstream file(AUTH_FILE);
    std::string line;
    std::string encryptedUsername = caesarCipher(username, 3);
    std::string encryptedPassword = caesarCipher(password, 3);
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string storedUsername, storedPassword;
        if (iss >> storedUsername >> storedPassword) {
            if (storedUsername == encryptedUsername && storedPassword == encryptedPassword) {
                return true;
            }
        }
    }
    return false;
}

bool addUser(const std::string& username, const std::string& password) {
    if (checkCredentials(username, password)) {
        return false;
    }
    std::ofstream file(AUTH_FILE, std::ios::app);
    std::string encryptedUsername = caesarCipher(username, 3);
    std::string encryptedPassword = caesarCipher(password, 3);
    file << encryptedUsername << " " << encryptedPassword << std::endl;
    return true;
}

void writeEncryptedChat(const std::string& clientName, const std::string& message) {
    std::ofstream file("Encrypted.txt", std::ios::app);
    std::string encryptedMessage = caesarCipher(message, 3);
    file << clientName << ": " << encryptedMessage << std::endl;
}

void handleClient(int clientId, SOCKET clientSocket) {
    char buffer[1024];
    bool isLoggedIn = false;
    std::string username, password;

    // Authentication loop
    while (!isLoggedIn) {
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cerr << "Client " << clientId << " disconnected.\n";
            closesocket(clientSocket);
            clients.erase(clientId);
            return;
        }

        buffer[bytesReceived] = '\0';
        std::istringstream iss(buffer);
        std::string command;
        iss >> command;

        if (command == "SIGNUP") {
            iss >> username >> password;
            if (addUser(username, password)) {
                isLoggedIn = true;
                send(clientSocket, "SIGNUP_SUCCESS", 14, 0);
            }
            else {
                send(clientSocket, "CREDENTIALS_EXISTS", 19, 0);
            }
        }
        else if (command == "LOGIN") {
            iss >> username >> password;
            if (checkCredentials(username, password)) {
                isLoggedIn = true;
                send(clientSocket, "LOGIN_SUCCESS", 13, 0);
            }
            else {
                send(clientSocket, "ACCESS_DENIED", 14, 0);
            }
        }
    }

    // Message handling loop
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cerr << "Client " << clientId << " disconnected.\n";
            closesocket(clientSocket);
            clients.erase(clientId);
            return;
        }

        buffer[bytesReceived] = '\0';
        std::string clientMessage = buffer; // This is the plain text message
        std::string clientName = clientNames[clientId];

        // Write the encrypted message to the file
        writeEncryptedChat(clientName, clientMessage);

        // Send plain text message to other clients
        for (const auto& pair : clients) {
            if (pair.first != clientId) {
                send(pair.second, clientMessage.c_str(), clientMessage.length(), 0);
            }
        }
    }
}


void listenForClients(SOCKET listeningSocket) {
    while (true) {
        sockaddr_in client;
        int clientSize = sizeof(client);

        SOCKET clientSocket = accept(listeningSocket, (struct sockaddr*)&client, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        int clientId = ++clientCounter;
        clients[clientId] = clientSocket;

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

        std::string clientName = "Client " + std::to_string(clientId);
        clientNames[clientId] = clientName;

        std::thread clientThread(handleClient, clientId, clientSocket);
        clientThread.detach();
    }
}

int main() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

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

    if (bind(listeningSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Status: Active :)\n";

    std::thread listenThread(listenForClients, listeningSocket);
    listenThread.join();

    closesocket(listeningSocket);
    WSACleanup();

    return 0;
}