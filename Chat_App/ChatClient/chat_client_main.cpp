#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <iostream>

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
};

std::atomic<bool> isRunning(true);

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

				std::cout << msg << "\n";
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
	std::string name;
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


	std::cout << "Conected to room as " << name << "...\n";
	std::cout << "Type '/exit' to leave the chat.\n";

	std::thread recieveThread(receiveMessage, serverSocket);

	while (isRunning) 
	{
		std::string input;
		std::getline(std::cin, input);

		if (input == "/exit") 
		{
			std::cout << "Exiting the chat..\n";
			isRunning.store(false, std::memory_order_relaxed);
			break;
		}

		ChatMessage message;
		message.message = "[" + name + "]: " + input;
		message.messageLength = message.message.length();
		message.header.messageType = 1;
		message.header.packetSize = message.messageLength + sizeof(message.messageLength) + sizeof(message.header.messageType) + sizeof(message.header.packetSize);

		const int bufSize = 512;
		Buffer buffer(bufSize);

		buffer.WriteUInt32LE(message.header.packetSize); 
		buffer.WriteUInt32LE(message.header.messageType);
		buffer.WriteUInt32LE(message.messageLength); 
		buffer.WriteString(message.message); 

		send(serverSocket, (const char*)(&buffer.m_BufferData[0]), message.header.packetSize, 0);
		std::cout << message.message << "\n";
	}

	freeaddrinfo(info);

	shutdown(serverSocket, SD_BOTH);
	closesocket(serverSocket);

	isRunning = false;

	WSACleanup();

	return 0;
}