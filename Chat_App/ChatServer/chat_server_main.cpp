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

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8412"



struct User
{
	SOCKET socket;
	std::string name;
	unsigned int ID;


};


struct Room
{
	std::string name;
	User* creator;
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
	return std::vector<User*>();
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
	message.message = msg;//"[" + name + "]: " + input;
	message.messageLength = message.message.length();
	message.header.messageType = 1;
	
	message.roomLength = 4;
	message.room = "main";
	message.nameLength = 6;
	message.userName = "Admin";
	message.header.packetSize = message.messageLength + sizeof(message.messageLength) + message.roomLength + sizeof(message.roomLength) + message.nameLength + sizeof(message.nameLength) + sizeof(message.header.messageType) + sizeof(message.header.packetSize);

	buffer.WriteUInt32LE(message.header.packetSize);
	buffer.WriteUInt32LE(message.header.messageType);
	buffer.WriteUInt32LE(message.messageLength);
	buffer.WriteString(message.message);
	buffer.WriteUInt32LE(message.roomLength);
	buffer.WriteString(message.room);
	buffer.WriteUInt32LE(message.nameLength);
	buffer.WriteString(message.userName);

	send(outSocket, (const char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);

}

User* GetUserBySocket(SOCKET socket)
{
	std::cout << socket;
	for(int i = 0; i<users.size(); i++ )
	{
		if (users[i]->socket == socket)
		{
			 
			return users[i];
		}
		
	}
	return nullptr;


}

void leaveRoom(std::string roomName, SOCKET socket, User* user) 
{


	//check if the room exist
	std::map<std::string, Room*>::iterator roomIt = roomMap.find(roomName);
	if (roomIt == roomMap.end())
	{
		SendMessageToSocket("Room not found: " + roomName, socket);
		return;
	}

	Room* room = roomIt->second;

	std::vector<User*>::iterator userIt = std::find(room->users.begin(), room->users.end(), user);
	if (userIt == room->users.end())
	{
		SendMessageToSocket("You are not in the room: " + roomName, socket);
		return;
	}

	room->users.erase(userIt);
	std::string leaveMsg = "You left the room: " + room->name;
	SendMessageToSocket(leaveMsg, socket);
}

int main(int arg, char** argv)
{
	// Initiliaze Winsock
	WSADATA wsaData;
	int result;

	// Set version 2.2 with MAKEWORD(2,2)
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed with error %d\n", result);
		return 1;
	}

	printf("WSAStartup successfully!\n");

	struct addrinfo* info = nullptr;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));  // ensure we dont have garbage data
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// Stream
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	hints.ai_flags = AI_PASSIVE;


	result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return 1;
	}

	printf("getaddrinfo was successful!\n");

	// Create the socket
	SOCKET listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}

	printf("socket created successfully!\n");

	result = bind(listenSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == SOCKET_ERROR)
	{
		printf("bind failed with error %d\n", result);
		closesocket(listenSocket);
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}

	printf("bind was successful!\n");

	// listen
	result = listen(listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR)
	{
		printf("listen failed with error %d\n", result);
		closesocket(listenSocket);
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}

	printf("listen was successful!\n");


	//std::vector<SOCKET> activeConnections;

	FD_SET socketsReadyForReading;		// list of all the clients ready to ready
	FD_ZERO(&socketsReadyForReading);

	// use a timeval to prevent select from waiting forever
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while (true)
	{
		FD_ZERO(&socketsReadyForReading);

		FD_SET(listenSocket, &socketsReadyForReading);

		for (int i = 0; i < users.size(); i++)
		{
			FD_SET(users[i]->socket, &socketsReadyForReading);
		}

		int count = select(0, &socketsReadyForReading, NULL, NULL, &tv);

		if (count == 0)
		{
			// Timevalue expired
			continue;
		}
		if (count == SOCKET_ERROR)
		{
			printf("select had an error %d\n", WSAGetLastError());
			continue;
		}

		for (int i = 0; i < users.size(); i++)
		{
			SOCKET socket = users[i]->socket;

			if (FD_ISSET(socket, &socketsReadyForReading))
			{
				const int bufSize = 512;
				Buffer buffer(bufSize);

				int result = recv(socket, (char*)(&buffer.m_BufferData[0]), bufSize, 0);

				if (result == SOCKET_ERROR)
				{
					printf("recv failed with error %d\n", WSAGetLastError());
					closesocket(socket);
					//activeConnections.erase(activeConnections.begin() + i); //NO PROPER CLEANING AFTER ERROR
					i--;
					continue;
				}
				else if (result == 0)
				{
					printf("Client disconnect");
					closesocket(socket);
					//activeConnections.erase(activeConnections.begin() + i);
					i--;
					continue;
				}

				uint32_t packetSize = buffer.ReadUInt32LE();
				uint32_t messageType = buffer.ReadUInt32LE();

				if (messageType == 1)
				{
					uint32_t messageLength = buffer.ReadUInt32LE();
					std::string msg = buffer.ReadString(messageLength);
					uint32_t roomLength = buffer.ReadUInt32LE();
					std::string room = buffer.ReadString(roomLength);
					uint32_t nameLength = buffer.ReadUInt32LE();
					std::string userName = buffer.ReadString(nameLength);

					User* user = nullptr;
					user = GetUserBySocket(socket);
					user->name = userName;
					printf("PacketSize:%d\nMessageType:%d\nMessageLength:%d\nMessage:%s\n", packetSize, messageType, messageLength, msg.c_str());

					if (msg[0] == '/')
					{
						std::stringstream msgStream(msg);
						std::string token;

						msgStream>>token;

						if (token == "/join")
						{
							msgStream >> token;
							std::string roomName = token;
							

							auto it = roomMap.find(roomName);  //Don't kill me for using auto, please. Spare my life. I can change it, but this format is so ugly.
							if (it != roomMap.end())
							{
								Room* currentRoom = it->second;
								currentRoom->users.push_back(users[i]);
								std::cout << "User joined room: " << roomName << std::endl;

								SendMessageToSocket("You joined room called " + roomName + ". It was created by " +currentRoom->creator->name, socket); //socket == User we got message from right now
							}
							else
							{
								Room* newRoom = new Room();
								newRoom->name = roomName;
								newRoom->users.push_back(users[i]);
								newRoom->creator = users[i];
								roomMap.insert(std::make_pair(newRoom->name, newRoom));
								std::cout << "User created room: " << roomName<<std::endl;

								SendMessageToSocket("You created new room called " + newRoom->name + ".", socket);
							}
						}

					

						if (token == "/leave")
						{

							msgStream >> token;
							std::string roomName = token;
							leaveRoom(roomName, socket, users[i]);
						}


						if (token == "/rooms")
						{
							std::string roomList = "Available rooms:\n";

							for (const auto& roomEntry : roomMap)
							{
								roomList += roomEntry.first + "\n";
							}

							if (roomMap.empty()) {
								roomList += "No rooms available.\n";
							}

							SendMessageToSocket(roomList, socket);
						}


						if (token == "/help")
						{
							std::string helpMessage =
								"Available commands:\n"
								"/rooms - List available rooms\n"
								"/join <room> - Create or join a room\n"
								"/leave <room> - Leave the current room\n"
								"/switch <room> - switch room you are writing to\n";

							SendMessageToSocket(helpMessage, socket);
						}



					}
					

					
					std::vector<User*> roomUsers;
					bool userInRoom = false;

					if (room == "main") roomUsers = users;
					else 
					{
						roomUsers = GetAllUsersInRoom(room);

					}

					for (int j = 0; j < roomUsers.size(); j++)
					{
						if (roomUsers[j] == users[i])
						{
								userInRoom = true;
								break;
						}
					}
					
					if (!userInRoom)
					{
						SendMessageToSocket("You are writing to a room you are not in.", socket);
					}
					else
					for (int j = 0; j < roomUsers.size(); j++)
					{
						SOCKET outSocket = roomUsers[j]->socket;

						if (outSocket != listenSocket && outSocket != socket)
						{

							std::cout << "Data send to user #" << j;
							send(outSocket, (const char*)(&buffer.m_BufferData[0]), packetSize, 0);
						}
					}

				}

				FD_CLR(socket, &socketsReadyForReading);
				count--;
			}
		}

		if (count > 0)
		{
			if (FD_ISSET(listenSocket, &socketsReadyForReading))
			{
				SOCKET newConnection = accept(listenSocket, NULL, NULL);
				if (newConnection == INVALID_SOCKET)
				{
					printf("accept failed with error %d\n", WSAGetLastError());
				}
				else
				{
					User* newUser = new User();
					newUser->ID = 123;
					newUser->socket = newConnection;
				
					users.push_back(newUser);
					
					//AddUserToMain(newUser);

					FD_CLR(listenSocket, &socketsReadyForReading);

					printf("Client connect with socket: %d\n", (int)newConnection);
				}
			}
		}
	}

	freeaddrinfo(info);
	closesocket(listenSocket);

	for (int i = 0; i < users.size(); i++)
	{
		closesocket(users[i]->socket);
	}

	WSACleanup();

	return 0;
}



