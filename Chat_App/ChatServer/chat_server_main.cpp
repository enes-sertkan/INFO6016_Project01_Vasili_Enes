#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <string>
#include "buffer.h"
#include <iostream>
#include <map>
#include <sstream>

#include <thread>
#include <mutex>
#include <queue>
#include "GUI.h"

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8412"

std::mutex messageMutex;
std::queue<std::string> messageQueue;

struct User
{
	SOCKET socket;
	unsigned int ID;

};


struct Room
{
	std::string name;
	std::vector<User*> users;
};

struct PacketHeader
{
	uint32_t packetSize;
	uint32_t messageType;
};

struct ChatMessage
{
	PacketHeader header;
	uint32_t messageLength;
	std::string message;
	uint32_t roomLength;
	std::string room;
	uint32_t nameLength;
	std::string userName;
};



std::vector<User*> users;
std::map<std::string, Room*> roomMap;

std::vector<User*> GetAllUsersInRoom(std::string roomName)
{
	auto it = roomMap.find(roomName);  //Don't kill me for using auto, please. Spare my life. I can change it, but this format is so ugly.
	if (it != roomMap.end())
	{
		return it->second->users;
	}
}

void PrepareMain()
{
	Room* newRoom = new Room();
	newRoom->name = "main";

	roomMap.insert(std::make_pair(newRoom->name, newRoom));
	std::cout << "main created." << std::endl;
}

void AddUserToMain(User* user)
{
	roomMap["main"]->users.push_back(user);
}

void SendMessageToSocket(std::string msg, SOCKET outSocket)
{
    const int bufSize = 512;
    Buffer buffer(bufSize);

    ChatMessage message;
    message.message = msg; // Format the message for the recipient
    message.messageLength = message.message.length();
    message.header.messageType = 1;

    message.roomLength = 4;
    message.room = "main";  // You can dynamically change the room name
    message.nameLength = 6;
    message.userName = "Admin"; // This could also be dynamic
    message.header.packetSize = message.messageLength
        + sizeof(message.messageLength)
        + message.roomLength
        + sizeof(message.roomLength)
        + message.nameLength
        + sizeof(message.nameLength)
        + sizeof(message.header.messageType)
        + sizeof(message.header.packetSize);

    buffer.WriteUInt32LE(message.header.packetSize);
    buffer.WriteUInt32LE(message.header.messageType);
    buffer.WriteUInt32LE(message.messageLength);
    buffer.WriteString(message.message);
    buffer.WriteUInt32LE(message.roomLength);
    buffer.WriteString(message.room);
    buffer.WriteUInt32LE(message.nameLength);
    buffer.WriteString(message.userName);

    // Send the buffer's data to the socket
    send(outSocket, (const char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
}


void StartServer() {
    // Initiliaze Winsock
    WSADATA wsaData;
    int result;

    // Set version 2.2 with MAKEWORD(2,2)
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed with error %d\n", result);
        return;
    }


    printf("WSAStartup successfully!\n");

    struct addrinfo* info = nullptr;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
    if (result != 0) {
        printf("getaddrinfo failed with error %d\n", result);
        WSACleanup();
        return;
    }

    printf("getaddrinfo was successful!\n");

    // Create the socket
    SOCKET listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error %d\n", WSAGetLastError());
        freeaddrinfo(info);
        WSACleanup();
        return;
    }

    printf("socket created successfully!\n");

    result = bind(listenSocket, info->ai_addr, (int)info->ai_addrlen);
    if (result == SOCKET_ERROR) {
        printf("bind failed with error %d\n", result);
        closesocket(listenSocket);
        freeaddrinfo(info);
        WSACleanup();
        return;
    }

    printf("bind was successful!\n");

    // listen
    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        printf("listen failed with error %d\n", result);
        closesocket(listenSocket);
        freeaddrinfo(info);
        WSACleanup();
        return;
    }

    printf("listen was successful!\n");

    FD_SET socketsReadyForReading;
    FD_ZERO(&socketsReadyForReading);

    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (true) {
        FD_ZERO(&socketsReadyForReading);
        FD_SET(listenSocket, &socketsReadyForReading);

        for (int i = 0; i < users.size(); i++) {
            FD_SET(users[i]->socket, &socketsReadyForReading);
        }

        int count = select(0, &socketsReadyForReading, NULL, NULL, &tv);

        if (count == 0) continue;
        if (count == SOCKET_ERROR) {
            printf("select had an error %d\n", WSAGetLastError());
            continue;
        }

        for (int i = 0; i < users.size(); i++) {
            SOCKET socket = users[i]->socket;

            if (FD_ISSET(socket, &socketsReadyForReading)) {
                const int bufSize = 512;
                Buffer buffer(bufSize);

                int result = recv(socket, (char*)(&buffer.m_BufferData[0]), bufSize, 0);

                if (result == SOCKET_ERROR) {
                    printf("recv failed with error %d\n", WSAGetLastError());
                    closesocket(socket);
                    i--;
                    continue;
                }
                else if (result == 0) {
                    printf("Client disconnect");
                    closesocket(socket);
                    i--;
                    continue;
                }

                uint32_t packetSize = buffer.ReadUInt32LE();
                uint32_t messageType = buffer.ReadUInt32LE();

                if (messageType == 1) {
                    uint32_t messageLength = buffer.ReadUInt32LE();
                    std::string msg = buffer.ReadString(messageLength);
                    uint32_t roomLength = buffer.ReadUInt32LE();
                    std::string room = buffer.ReadString(roomLength);
                    uint32_t nameLength = buffer.ReadUInt32LE();
                    std::string userName = buffer.ReadString(nameLength);

                    printf("PacketSize:%d\nMessageType:%d\nMessageLength:%d\nMessage:%s\n", packetSize, messageType, messageLength, msg.c_str());

                    // Check if the message is a command
                    if (msg[0] == '/') {
                        std::stringstream msgStream(msg);
                        std::string token;

                        msgStream >> token;

                        // Handle commands like /join or /send
                        if (token == "/join") {
                            // Join room logic
                            msgStream >> token;
                            std::string roomName = token;

                            auto it = roomMap.find(roomName);
                            if (it != roomMap.end()) {
                                Room* currentRoom = it->second;
                                currentRoom->users.push_back(users[i]);
                                std::cout << "User joined room: " << roomName << std::endl;

                                SendMessageToSocket("You joined room called " + roomName + ".", socket);
                            }
                            else {
                                Room* newRoom = new Room();
                                newRoom->name = roomName;
                                newRoom->users.push_back(users[i]);
                                roomMap.insert(std::make_pair(newRoom->name, newRoom));
                                std::cout << "User created room: " << roomName << std::endl;

                                SendMessageToSocket("You created new room called " + newRoom->name + ".", socket);
                            }
                        }
                        else if (token == "/send") {
                            // Sending message to a room logic
                            msgStream >> token;
                            std::string roomName = token;
                            msgStream >> token;
                            std::string message = token;
                            msg = message;

                            std::vector<User*> roomUsers = GetAllUsersInRoom(roomName);

                            for (int j = 0; j < roomUsers.size(); j++) {
                                SOCKET outSocket = roomUsers[j]->socket;

                                if (outSocket != listenSocket && outSocket != socket) {
                                    std::cout << "Data sent to user #" << j << std::endl;
                                    SendMessageToSocket(msg, outSocket);
                                }
                            }
                        }
                    }

                    // Broadcast message to users in the same room
                    std::vector<User*> roomUsers;
                    if (room == "main") {
                        roomUsers = users;  // Send to all

        }

        if (count > 0) {
            if (FD_ISSET(listenSocket, &socketsReadyForReading)) {
                SOCKET newConnection = accept(listenSocket, NULL, NULL);
                if (newConnection == INVALID_SOCKET) {
                    printf("accept failed with error %d\n", WSAGetLastError());
                }
                else {
                    User* newUser = new User();
                    newUser->ID = 123;
                    newUser->socket = newConnection;
                    users.push_back(newUser);

                    FD_CLR(listenSocket, &socketsReadyForReading);

                    printf("Client connect with socket: %d\n", (int)newConnection);
                }
            }
        }
    }

    freeaddrinfo(info);
    closesocket(listenSocket);

    for (int i = 0; i < users.size(); i++) {
        closesocket(users[i]->socket);
    }

    WSACleanup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Correct instantiation of GUI
    GUI gui(hInstance, nCmdShow); // Ensure parameters are passed

    // Initialize the GUI
    if (!gui.Initialize()) {
        return -1; // Initialization failed
    }

    // Start the server in a separate thread
    std::thread serverThread(StartServer);

    // Run the GUI message loop
    gui.RunMessageLoop(); // Start processing messages and updating the GUI

    // Wait for the server thread to finish
    serverThread.join();

    return 0; // Exit application
}

int main(int argc, char** argv) {
    // This is a console application; you might not have HINSTANCE and nCmdShow.
    // You can modify the GUI constructor to handle this case or pass default values.
    HINSTANCE hInstance = GetModuleHandle(NULL);  // Get current module handle for console applications.
    int nCmdShow = SW_SHOWDEFAULT;

    // Pass the correct parameters to the GUI constructor
    GUI myGui(hInstance, nCmdShow);

    if (!myGui.Initialize()) {
        std::cerr << "Failed to initialize GUI." << std::endl;
        return -1;
    }

    std::thread serverThread(StartServer);

    while (myGui.IsRunning()) {
        myGui.Update();

        if (myGui.HasNewMessage()) {
            std::string message = myGui.GetNewMessage();
            {
                std::lock_guard<std::mutex> lock(messageMutex);
                messageQueue.push(message);
            }
        }

        {
            std::lock_guard<std::mutex> lock(messageMutex);
            if (!messageQueue.empty()) {
                std::string message = messageQueue.front();
                messageQueue.pop();
                myGui.DisplayMessageFromServer(message);
            }
        }

    }

    serverThread.join();
    return 0;
}
