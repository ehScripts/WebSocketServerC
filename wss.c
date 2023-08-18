#include <stdio.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include "wss.h"
#include "http_parser.h"
#include "http_parser.c"
#include "sha1.h"
#include "sha1.c"
#include "base64.h"
#include "base64.c"

#define SA struct sockaddr

typedef struct WebSocketConnection
{
    int isOpen;
    int socketId;
    int sockfd;
} WebSocketConnection;

typedef struct WebSocketServer
{
    int port;
    int socketId;
    WebSocketConnection sockets[];
} WebSocketServer;

void wss_listen(WebSocketServer wss)
{
    wss.socketId = -1;
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    for (;;)
    {
        // socket create and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            printf("socket creation failed...\n");
        }
        else
            printf("Socket successfully created..\n");
        bzero(&servaddr, sizeof(servaddr));

        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(wss.port);

        int one = 1;

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        // Binding newly created socket to given IP and verification
        if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
        {
            printf("socket bind failed...\n");
        }
        else
            printf("Socket successfully binded..\n");

        // Now server is ready to listen and verification
        if ((listen(sockfd, 5)) != 0)
        {
            printf("Listen failed...\n");
            exit(0);
        }
        else
            printf("Server listening..\n");

        socklen_t len = sizeof(cli);

        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA *)&cli, &len);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            exit(0);
        }
        else
        {
            printf("server accept the client...\n");
        };

        wss.socketId++;
        WebSocketConnection thisConnection;
        thisConnection.socketId = wss.socketId;
        wss.sockets[wss.socketId] = thisConnection;
        wss.sockets[wss.socketId].isOpen = 1;
        wss.sockets[wss.socketId].sockfd = sockfd;

        while (wss.sockets[thisConnection.socketId].isOpen)
        {
            char buf[2048];
            int n;

            size_t recved = recv(connfd, buf, 2048, 0);

            printf("--- START OF RAW REQUEST ---\n%s\n--- END OF RAW REQUEST ---\n", buf);

            if (recved < 0)
            {
                printf("err");
            };

            HTTPRequest parsed = parseHTTP(buf);

            printf("UPGRADE: %s\n", parsed.upgrade);

            if (!strcmp(parsed.upgrade, "websocket"))
            {
                char concatString[128];
                strcpy(concatString, parsed.sec_websocket_key);
                strcat(concatString, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

                char sha1Result[128];
                SHA1(&sha1Result, concatString, strlen(concatString));

                char base64Result[128];
                Base64encode(&base64Result, sha1Result, strlen(sha1Result));

                char switchingProtocols[256];
                sprintf(switchingProtocols, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", base64Result);

                printf("Switching protocols... [Accept: %s]\n", base64Result);
                int bytes_sent = send(connfd, switchingProtocols, strlen(switchingProtocols), 0);

                if (bytes_sent == strlen(switchingProtocols))
                {
                    printf("%d bytes sent\n", bytes_sent);
                }
                else
                {
                    perror("Error sending response");
                }
            }
            else
            {
                char response[] = "HTTP/1.1 426 Upgrade Required\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 16\r\nConnection: keep-alive\r\nKeep-Alive: timeout=5\r\n\nUpgrade Required";
                int bytes_sent = send(connfd, response, sizeof(response), 0);
                printf("%d\n", bytes_sent);
                close(sockfd);
                wss.sockets[thisConnection.socketId].isOpen = 0;
            };
        };
    };
};