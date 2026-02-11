#include "emtrace/emtrace.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

static void out(const void* data, size_t size, int connfd) { (void) write(connfd, data, size); }
static void begin(const void* info, size_t total_size, int connfd) {
    (void) total_size;
    uintptr_t ptr = (uintptr_t) info;
    out(&ptr, sizeof(ptr), connfd);
}

#define DUMMY(x, y, z) ((void) 0)

#define TRACEF(connfd, ...)                                                                        \
    EMT_TRACE_F(                                                                                   \
        __attribute__((used)) __attribute__((section(".emtrace"))) const, EMT_PY_FORMAT, uint16_t, \
        out, begin, DUMMY, connfd, "", __VA_ARGS__                                                 \
    )

int main(void) {
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr;
    struct sockaddr_in cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else {
        printf("Socket successfully created..\n");
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (SA*) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    } else {
        printf("Socket successfully binded..\n");
    }

    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    } else {
        printf("Server listening..\n");
    }
    len = sizeof(cli);

    while (true) {
        int connfd = accept(sockfd, (SA*) &cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        } else {
            printf("server accept the client...\n");
        }

        static emt_magic_t magic =
            EMT_MAGIC(EMT_ENCODING_NONE, size_t, EMT_DEFAULT_ALIGNMENT_POWER);

        EMT_INIT(&magic, begin, DUMMY, connfd);
        int x = 1;
        int y = 2;
        for (int i = 0; i < 15; i++) {
            TRACEF(connfd, "Hello, World! {:d}", VAL(int, y));
            TRACEF(connfd, "  test\n");
            TRACEF(
                connfd, "Hello, World! 0x{0:x} {2:d} {1:d}\n", //
                VAL(int, i), VAL(int, 'a'), VAL(void*, &x)
            );
            TRACEF(connfd, "{:-^20d}\n", VAL(int, i));
            for (int j = i; j > 3; j--) {
                TRACEF(connfd, "|{:^18d}|\n", VAL(int, j));
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
