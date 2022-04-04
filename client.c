//
// Created by shenk on 04.04.2022.
//

#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include "common_structs.h"

int main(int argc, char  **argv) {
    int sock_d = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sock_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(31415)
    };
    inet_aton("127.0.0.1", &sock_addr.sin_addr);

    int err = connect(sock_d, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (err < 0) {
        printf("Error: can't connect to server - %d", err);
    }

    struct init_msg msg = {
            .save_filename = "hello_world.txt",
            .file_size = 300
    };

    err = send(sock_d, &msg, sizeof(struct init_msg), 0);
    if (err < 0) {
        printf("Error: can't send data - %d", err);
    }

    return 0;
}