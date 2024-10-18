#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <sstream>>
#include "string"
#include "buffer.h"

// Need to link Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8412"

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

std::atomic<bool> isRunning(true);
std::string currentRoom = "main";
std::string name;
struct addrinfo* info = nullptr;

void receiveMessage(SOCKET socket)
{

	while (isRunning.load(std::memory_order_relaxed))
	{
		const int bufSize = 512;
		Buffer buffer(bufSize);
		int result = recv(socket, (char*)(&buffer.m_BufferData[0]), bufSize, 0);
		if (result > 0)
		{
			uint32_t packetSize = buffer.ReadUInt32LE();
			uint32_t messageType = buffer.ReadUInt32LE();

			if (messageType == 1)
			{
				// handle the message
				uint32_t messageLength = buffer.ReadUInt32LE();
				std::string msg = buffer.ReadString(messageLength);
				uint32_t roomLength = buffer.ReadUInt32LE();
				std::string room = buffer.ReadString(roomLength);
				uint32_t nameLength = buffer.ReadUInt32LE();
				std::string userName = buffer.ReadString(nameLength);

				printf("\33[2K\r");
				std::cout <<"["<<room<<"] "<< userName<<":" << msg << "\n";
				std::cout << "[" << currentRoom << "] " + name+":";
			}
		}
		else if (result == 0)
		{
			std::cout << "Server closed the connection.\n";
			break;
		}
		else {
			printf("recv failed with error %d\n", WSAGetLastError());
			break;
		}
	}
}





SOCKET PrepareClient()
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

	
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));  // ensure we dont have garbage data
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// Stream
	hints.ai_protocol = IPPROTO_TCP;	// TCP
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &info);
	if (result != 0)
	{
		printf("getaddrinfo failed with error %d\n", result);
		WSACleanup();
		return 1;
	}

	printf("getaddrinfo was successful!\n");

	// Create the server socket
	SOCKET serverSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("socket failed with error %d\n", WSAGetLastError());
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}

	printf("socket created successfully!\n");


	std::cout << "Enter your name: ";

	std::getline(std::cin, name);

	// Connect
	result = connect(serverSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == INVALID_SOCKET)
	{
		printf("connect failed with error %d\n", WSAGetLastError());
		closesocket(serverSocket);
		freeaddrinfo(info);
		WSACleanup();
		return 1;
	}

	printf("Connect to the server successfully!\n");

	return serverSocket;
}

int main(int arg, char** argv)
{
	SOCKET serverSocket = PrepareClient();

	std::cout << "Conected to room as " << name << "...\n";
	std::cout << "Type '/exit' to leave the chat.\n";

	std::thread recieveThread(receiveMessage, serverSocket);

	while (isRunning)
	{
		std::string input;
		//std::cout << "\n[" + currentRoom + "] ";
		std::getline(std::cin, input);

		if (input == "/exit")
		{
			std::cout << "Exiting the chat..\n";
			isRunning.store(false, std::memory_order_relaxed);
			break;
		}
		if (input[0] == '/')
		{
			std::stringstream msgStream(input);
			std::string token;

			msgStream >> token;


			//I know this is a VERY BAD WAY to do commands. They should be on server or client but definetly not on both.
			//But we are learning this time and this is simple enough so it will work
			
			if (token == "/join")
			{
				msgStream >> token;
				currentRoom = token;
			}

			if (token == "/switch")
			{
				
				msgStream >> token;
				currentRoom = token;
				std::cout << "You switched your room to" + currentRoom;
			}

			if (token == "/leave")
			{
				//std::cout << "switched";
				
				currentRoom = "main";
			}

		}


			ChatMessage message;
			message.message = input;//"[" + name + "]: " + input;
			message.messageLength = message.message.length();
			message.header.messageType = 1;
			message.header.packetSize = message.messageLength + sizeof(message.messageLength) + message.roomLength + sizeof(message.roomLength) + message.nameLength + sizeof(message.nameLength) + sizeof(message.header.messageType) + sizeof(message.header.packetSize);
			message.roomLength = currentRoom.length();
			message.room = currentRoom;
			message.nameLength = name.length();
			message.userName = name;

			const int bufSize = 512;
			Buffer buffer(bufSize);

			buffer.WriteUInt32LE(message.header.packetSize);
			buffer.WriteUInt32LE(message.header.messageType);
			buffer.WriteUInt32LE(message.messageLength);
			buffer.WriteString(message.message);
			buffer.WriteUInt32LE(message.roomLength);
			buffer.WriteString(message.room);
			buffer.WriteUInt32LE(message.nameLength);
			buffer.WriteString(message.userName);


			send(serverSocket, (const char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
			//std::cout << message.message << "\n";
			printf("\33[2K\r");
			std::cout << "[" << currentRoom << "] " + name + ":";
		}



		freeaddrinfo(info);

		shutdown(serverSocket, SD_BOTH);
		closesocket(serverSocket);

		isRunning = false;

		WSACleanup();

		return 0;
	}
