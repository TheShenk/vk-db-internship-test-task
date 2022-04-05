//
// Created by shenk on 04.04.2022.
//

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "common_structs.h"

int main(int argc, char  **argv) {

    char *addr = "127.0.0.1";
    unsigned int port = 31415;

    char *filename = NULL;
    char *save_filename = NULL;

    char *opts = "-a:p:s:";

    char opt = getopt(argc, argv, opts);
    while (opt > 0) {
        switch (opt) {
            case 'a':
                addr = optarg;
                break;
            case 'p':
                port = strtol(optarg, NULL, 10);
                break;
            case 's':
                save_filename = optarg;
                break;
            case '\1':
                filename = optarg;
                break;
        }
        opt = getopt(argc, argv, opts);
    }

    if (!filename) {
        printf("Usage: %s [-a address] [-p port] [-s save filename] filename\n", argv[0]);
        return 0;
    }
    if (!save_filename) save_filename = filename;

    printf("File to send: %s\n", filename);
    printf("Save as: %s\n", save_filename);

    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_d < 0) {
        printf("Error: can't create socket - %d\n", errno);
        return sock_d;
    }

    struct sockaddr_in sock_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port)
    };
    inet_aton(addr, &sock_addr.sin_addr);

    int err = connect(sock_d, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (err < 0) {
        printf("Error: can't connect to server - %d\n", errno);
        return err;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: can't open file\n");
        return -1;
    }

    int file_d = fileno(file);

    struct stat buf;
    fstat(file_d, &buf);
    off_t size = buf.st_size;

    struct init_msg msg = {
            .file_size = size
    };

    strncpy(msg.save_filename, save_filename, MAX_FILENAME_LEN-1);
    msg.save_filename[MAX_FILENAME_LEN-1] = '\0';

    ssize_t init_sent_size = send(sock_d, &msg, sizeof(struct init_msg), 0);
    if (init_sent_size < 0) {
        printf("Error: can't send init data - %d\n", errno);
        return init_sent_size;
    }

    off_t offset = 0;
    ssize_t sent_size = sendfile(sock_d, file_d, &offset, size);
    printf("Sent %zd bytes\n", sent_size);

    fclose(file);

    return 0;
}