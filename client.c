//
// Created by shenk on 04.04.2022.
//

#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char  **argv) {
    int sock_d = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sock_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(31415)
    };
    inet_aton("127.0.0.1", &sock_addr.sin_addr);

    connect(sock_d, (struct sockaddr *)&sock_addr, sizeof(sock_addr));

    return 0;
}