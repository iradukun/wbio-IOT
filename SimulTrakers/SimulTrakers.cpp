#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#if 1
#define USE_CODEC8_EXTENDED
#endif
#ifdef USE_CODEC8_EXTENDED
#include "Codec8Extended.h"
#else
#include "Codec8.h"
#endif
#include <string>
#include <process.h>
#include <sstream>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#if 0
constexpr int nNbThreads = MAXIMUM_WAIT_OBJECTS;
constexpr int nNbClientsPerThread = 100;
constexpr int nNbDataToSend = 10;
#else
constexpr int nNbThreads = 1;
constexpr int nNbClientsPerThread = 1;
constexpr int nNbDataToSend = 1;
#endif

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5000"

struct TThreadData
{
    struct addrinfo* result;
    uint64_t nBaseIMEI;
};


void __cdecl thread(TThreadData *pData)
{
    struct addrinfo * ptr = NULL;
    int iResult;

#ifdef USE_CODEC8_EXTENDED
    CCodec8Extended* pProtos[nNbClientsPerThread] = { 0 };
#else
    CCodec8* pProtos[nNbClientsPerThread] = { 0 };
#endif

    for (int i = 0; i < nNbClientsPerThread; ++i)
    {
        SOCKET ConnectSocket = INVALID_SOCKET;
        // Attempt to connect to an address until one succeeds
        for (ptr = pData->result; ptr != NULL; ptr = ptr->ai_next)
        {
            // Create a SOCKET for connecting to server
            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                ptr->ai_protocol);
            if (ConnectSocket == INVALID_SOCKET)
            {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                return;
            }

            // Connect to server.
            iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR)
            {
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }

        if (ConnectSocket == INVALID_SOCKET)
        {
            printf("Unable to connect to server!\n");
            break;
        }
        std::ostringstream oss;
        oss << uint64_t(pData->nBaseIMEI + i * 3);

#ifdef USE_CODEC8_EXTENDED
            pProtos[i] = new CCodec8Extended(oss.str().c_str(), ConnectSocket);
#else
            pProtos[i] = new CCodec8(oss.str().c_str(), ConnectSocket);
#endif
    }

    for (int i = 0 ; i < nNbClientsPerThread; ++i)
    {
        if (pProtos[i])
        {
            pProtos[i]->Start();
        }
    }

    Sleep(1000);

    for (int j = 0; j < nNbDataToSend; ++j)
    {
        for (int i = 0 ; i < nNbClientsPerThread; ++i)
        {
            if (pProtos[i])
            {
                pProtos[i]->SendPosition(55.4 + i, 25.7 - i, 5.3, 0, 0.);
            }
        }

        Sleep(1000);
    }

    for (int i = 0 ; i < nNbClientsPerThread; ++i)
    {
        if (pProtos[i])
        {
            pProtos[i]->Stop();
            delete pProtos[i];
        }
    }

    delete pData;
}

int __cdecl main(int argc, char** argv)
{
    WSADATA wsaData;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    int iResult;

    // Validate the parameters
    if (argc != 2)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }


    HANDLE hThreads[nNbThreads] = { 0 };

    for (int i = 0; i < nNbThreads; ++i)
    {
        TThreadData* pData = new TThreadData;
        pData->result = result;
        pData->nBaseIMEI = 861629054005354 + i * 13 * nNbClientsPerThread;

        uintptr_t handle = _beginthread((void (*)(void*))thread, 0, pData);
        if (handle == -1)
        {
            delete pData;
            continue;
        }
        hThreads[i] = (HANDLE)handle;
    }

    WaitForMultipleObjects(nNbThreads, hThreads, TRUE, INFINITE);

    freeaddrinfo(result);

    printf("Bye!\n");

    WSACleanup();

    return 0;
}