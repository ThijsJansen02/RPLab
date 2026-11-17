#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <Engine/Engine.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8000"

using namespace PH;

void closeSocketimpl(uint32 sock) {
    closesocket((SOCKET)sock);
}

uint32 connectToSocket(const char* hostname, const char* message)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = message;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        Engine::WARN << "WSAStartup failed with error: " << iResult <<  "\n";
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(hostname, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        Engine::INFO << "getaddrinfo failed with error: " << iResult << "\n";
        WSACleanup();
        return 1;
    }


    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

        char ipstr[INET_ADDRSTRLEN];

        sockaddr_in* addr = (sockaddr_in*)ptr->ai_addr;

        inet_ntop(
            AF_INET,
            &addr->sin_addr,
            ipstr,
            sizeof(ipstr)
        );

        Engine::INFO << "Trying to connect to " << ipstr
            << ":" << ntohs(addr->sin_port) << "\n";

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {

            int error = WSAGetLastError();

            char* message = nullptr;

            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&message,
                0,
                nullptr
            );
            Engine::WARN << "socket failed with error: " << message << "\n";

            WSACleanup();
            continue;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            int error = WSAGetLastError();

            char* message = nullptr;

            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&message,
                0,
                nullptr
            );
            Engine::WARN << "socket failed with error: " << message << "\n";

            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);
    Engine::INFO << "succesfully connected to socket!\n";
    return ConnectSocket;
}

void sendMessageOverSocket(uint32 connect, const char* message) {
    
    
    if (connect == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return;
    }
    int iResult;
    const char* sendbuf = message;

    // send buffer
    iResult = send(connect, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        Engine::WARN << "send to socket failed" << "\n";
        closesocket(connect);
        WSACleanup();
    }

    Engine::INFO << "Bytes Sent: " << iResult << "\n";
}