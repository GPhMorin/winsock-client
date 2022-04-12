#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <Mswsock.h>
#include <fstream>
#include <vector>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 500000
#define DEFAULT_PORT "33333"

using namespace std;

int __cdecl main(int argc, char** argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
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

    string cmd;
    bool exiting = false;

    do
    {
        cout << ">>> ";
        getline(cin, cmd);
        if (cmd == "exit()\0") {
            exiting = true;
        }
        else if (cmd[0] == 's' || cmd[0] == 'S' && cmd[1] == 'e' || cmd[1] == 'E' && cmd[2] == 'n' || cmd[3] == 'N' && cmd[3] == 'd' || cmd[4] == 'D' && cmd[4] == ' ') {
            string location = cmd;
            location.erase(0, 5);

            // https://stackoverflow.com/a/36659103
            // open file
            ifstream infile(location);
            vector<char> buffer;

            // get length of file
            infile.seekg(0, infile.end);
            size_t length = infile.tellg();
            infile.seekg(0, infile.beg);

            int nb = length / DEFAULT_BUFLEN + 1;
            int ln = 18 + log10(nb);

            iResult = send(ConnectSocket, "FILETRANSMISSION " + nb, ln, 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes Sent: %ld\n", iResult);

            //read file
            if (length > 0) {
                buffer.resize(length);
                infile.read(&buffer[0], length);
            }

            int i = 1;
            string part = "";
            for (auto it : buffer) {
                part += it;
                if (i % DEFAULT_BUFLEN == 0) {
                    iResult = send(ConnectSocket, part.c_str(), strlen(part.c_str()), 0);
                    if (iResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                    }
                    printf("Bytes Sent: %ld\n", iResult);

                    i = 0;
                    part.clear();
                }
                i++;
            }
            iResult = send(ConnectSocket, part.c_str(), strlen(part.c_str()), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes Sent: %ld\n", iResult);
        }
        else {
            iResult = send(ConnectSocket, cmd.c_str(), (int)strlen(cmd.c_str()), 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes Sent: %ld\n", iResult);

            iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                printf("Bytes received: %d\n", iResult);
                for (int i = 0; i < iResult; i++) {
                    cout << recvbuf[i];
                }
                cout << endl;
            }
            else if (iResult == 0)
                printf("Connection closed\n");
            else
                printf("recv failed with error: %d\n", WSAGetLastError());
        }
        
    } while (exiting == false);    


    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}