//
// Created by shenk on 04.04.2022.
//

#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char  **argv) {
    int sock_d = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(31415),
            .sin_addr = {
                    .s_addr = INADDR_ANY
            }
    };

    int err = bind(sock_d, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        printf("Error: can't bind socket - %d", err);
    }
    err = listen(sock_d, 1);
    if (err < 0) {
        printf("Error: can't listen - %d", err);
    }

    setsockopt(sock_d, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    while (1) {
        int conn_sock_d = accept(sock_d, NULL, NULL);
        if (conn_sock_d < 0) {
            printf("Error: can't accept - %d", err);
        }
    }

    return 0;
}