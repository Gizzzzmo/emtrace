#include "emtrace/emtrace.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

void out(const void* data, size_t size, int connfd) { (void)write(connfd, data, size); }

#define dummy(x, y, z) ((void)0)

#define TRACEF(connfd, ...)                                                                                            \
    EMTRACE_F_(                                                                                                        \
        __attribute__((used)) __attribute__((section(".emtrace"))) static const, EMTRACE_PY_FORMAT, out, dummy, dummy, \
        connfd, "", __VA_ARGS__                                                                                        \
    )

// Driver function
int main() {
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    } else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    } else
        printf("Server listening..\n");
    len = sizeof(cli);

    while (true) {
        // Accept the data packet from client and verification
        int connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        } else
            printf("server accept the client...\n");

        EMTRACE_INIT_(EMTRACE_DEFAULT_SEC_ATTR, out, connfd);
        int x = 1;
        int y = 2;
        void* ptr = &x;
        // TRACE("Hello, World!");
        for (int i = 0; i < 15; i++) {
            TRACEF(connfd, "Hello, World! {:d}", int8_t, y);
            TRACEF(connfd, "  test\n");
            TRACEF(connfd, "Hello, World! 0x{0:x} {2:d} {1:d}\n", int8_t, i, int8_t, 'a', void*, &x);
            TRACEF(connfd, "{:-^20d}\n", uint8_t, i);
            for (int j = i; j > 3; j--) {
                TRACEF(connfd, "|{:^18d}|\n", uint8_t, j);
            }
            TRACEF(
                connfd, "--------------------\n"
                        "|                  |\n"
                        "--------------------\n"
            );
        }

        TRACEF(connfd, "Hello World!\n");
        close(connfd);
    }

    // After chatting close the socket
    close(sockfd);
}
