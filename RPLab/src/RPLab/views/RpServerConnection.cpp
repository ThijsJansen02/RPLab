#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "RpServerConnection.h"

#include <Base/Datastructures/Stream.h>

#include <Engine/Engine.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8000"

using namespace PH;

namespace RPLab {

    void uploadServerFileCallback(LabCoreSSH::SFTPwrite* write, LabCoreSSH::SFTPwriteResult* result) {

        if (result->writesucces) {
            Engine::INFO << "succesfully uploaded Server file: " << write->path.getC_Str();
        }
        else {
            Engine::WARN << "Failed to uploaded Server file: " << write->path.getC_Str();
        }
    }

    //builds the server on the red pitaya
    void buildServerOnRedPitaya(const char* localsourcepath, PH::LabCoreSSH::Connection* redpitaya) {

        if (!redpitaya->connected) {
            Engine::WARN << "cant build the server on rp because the connection is not valid!\n";
            return;
        }

        auto localserverpath = Engine::String::create(localsourcepath);
        auto serversource = Engine::String::create(localsourcepath).append("RPLab-server.cpp");
        auto servermakefile = Engine::String::create(localsourcepath).append("Makefile");

        LabCoreSSH::uploadFile(redpitaya, serversource.getC_Str(), "RPLab/RPLab-server.cpp", uploadServerFileCallback);
        LabCoreSSH::uploadFile(redpitaya, servermakefile.getC_Str(), "RPLab/Makefile", uploadServerFileCallback);

        Engine::INFO << LabCoreSSH::sendShellCommand_synchronous(redpitaya, "cd RPLab; make", 10000).getC_Str() << "\n";
    }

    void ServerRunCallback(LabCoreSSH::SSHshellExec* exec, Base::SubString out, Base::SubString err) {

        Engine::INFO << out << "\n";
        Engine::ERR << err << "\n";
    }

    //runs the RPLab server on the red pitaya, consumes the connection
    bool32 runServer(PH::LabCoreSSH::Connection* redpitaya) {
        if (!redpitaya->connected) {
            Engine::WARN << "red pitaya not connected\n";
            return false;
        }

        LabCoreSSH::sendShellCommand(redpitaya, "./RPLab/RPLab-server", ServerRunCallback);
    }

    //returns a socket connection to the red pitaya server
    uint32 connectToServer(const char* hostname, const char* message)
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
            Engine::WARN << "WSAStartup failed with error: " << iResult << "\n";
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
        return ConnectSocket;
    }

    enum RequestType {
        CLOSE_CONNECTION = 0,
        CLOSE_SERVER = 1,
        RUN_BITFILE = 2,
        SET_COEFFICIENTS = 3
    };

    struct RequestHeader {
        RequestType type;
        uint32 payload;
        uint32 eor;
    };

    void sendMessageOverSocket(uint32 socket, Base::Array<uint8> ms);

    void sendCoefficientsToServer(PH::uint32 serversocket, Base::Array<CoefficientWritePair> coefficients) {
        RequestHeader h;
        h.type = RequestType::SET_COEFFICIENTS;
        h.payload = sizeof(RequestHeader);
        h.eor = sizeof(RequestHeader) + coefficients.count * sizeof(CoefficientWritePair);

        for (auto c : coefficients) {
            Engine::INFO << "addr: " << c.address << " value: " << c.value;
        }

        Base::Stream<Engine::Allocator> s = Base::Stream<Engine::Allocator>::create(sizeof(RequestHeader));

        s << h;
        s << coefficients;

        sendMessageOverSocket(serversocket, s.getArray());

        Base::Stream<Engine::Allocator>::destroy(&s);
    }


    void closeServerConnection(uint32 serversocket) {
        Base::Stream<Engine::Allocator> s = Base::Stream<Engine::Allocator>::create(sizeof(RequestHeader));
        RequestHeader h{};
        h.type == CLOSE_CONNECTION;
        h.payload = sizeof(RequestHeader);
        h.eor = sizeof(RequestHeader);
        s << h;
        if (serversocket > 1) {
            sendMessageOverSocket(serversocket, s.getArray());
        }

        Base::Stream<Engine::Allocator>::destroy(&s);

        closesocket((SOCKET)serversocket);
    }

    void sendRunBitFileRequest(const char* bitfilepath, uint32 socket) {
        Base::Stream<Engine::Allocator> s = Base::Stream<Engine::Allocator>::create(30);
        RequestHeader h;
        h.type = RUN_BITFILE;
        h.payload = sizeof(h);
        h.eor = sizeof(h) + Base::stringLength(bitfilepath) + 1;
        s << h;
        s << bitfilepath;
        
        sendMessageOverSocket(socket, s.getArray());

        Base::Stream<Engine::Allocator>::destroy(&s);
    }

    void sendServerClose(uint32 socket) {
        Base::Stream<Engine::Allocator> s = Base::Stream<Engine::Allocator>::create(sizeof(RequestHeader));
        RequestHeader h{};
        h.type = CLOSE_SERVER;
        h.payload = sizeof(RequestHeader);
        h.eor = sizeof(RequestHeader);
        s << h;
        sendMessageOverSocket(socket, s.getArray());

        Base::Stream<Engine::Allocator>::destroy(&s);
    }

    void sendMessageOverSocket(uint32 socket, Base::Array<uint8> ms) {
        
        if (socket == INVALID_SOCKET) {
            printf("Unable to connect to server!\n");
            WSACleanup();
            return;
        }

        int iResult;
        uint8* sendbuf = ms.data;

        // send buffer
        iResult = send(socket, (const char*)sendbuf, ms.count, 0);
        if (iResult == SOCKET_ERROR) {
            Engine::WARN << "send to socket failed" << "\n";
            closesocket(socket);
            WSACleanup();
        }

        Engine::INFO << "Bytes Sent: " << iResult << "\n";
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
}

