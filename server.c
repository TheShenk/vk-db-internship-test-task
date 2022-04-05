//
// Created by shenk on 04.04.2022.
//

#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>

#include "common_structs.h"

#define READ_DATA_CHUNK 255

int main(int argc, char  **argv) {

    unsigned int port = 31415;
    char *save_dir = "./";
    char *opts = "p:d:";

    char opt = getopt(argc, argv, opts);
    while (opt > 0) {
        switch (opt) {
            case 'p':
                port = strtoul(optarg, NULL, 10);
                break;
            case 'd':
                save_dir = optarg;
                break;
        }
        opt = getopt(argc, argv, opts);
    }

    printf("Listen port: %d\n", port);
    printf("Saving to: %s\n", save_dir);

    struct stat dir = {0};
    if(stat(save_dir, &dir) == -1)
    {
        int mkdir_err = mkdir(save_dir, 0755);
        if (mkdir_err < 0) {
            printf("Error: can't create directory - %d\n", errno);
            return mkdir_err;
        } else {
            printf("Directory created\n");
        }
    }

    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_d < 0) {
        printf("Error: can't create socket - %d\n", errno);
        return sock_d;
    }

    struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {
                    .s_addr = INADDR_ANY
            }
    };

    int err = bind(sock_d, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        printf("Error: can't bind socket - %d\n", errno);
        return err;
    }

    err = listen(sock_d, 1);
    if (err < 0) {
        printf("Error: can't listen - %d\n", errno);
        return err;
    }

    int val = 1;
    setsockopt(sock_d, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    while (1) {
        int conn_sock_d = accept(sock_d, NULL, NULL);
        if (conn_sock_d < 0) {
            printf("Error: can't accept - %d\n", errno);
            continue;
        }

        struct init_msg msg;
        ssize_t recv_size = recv(conn_sock_d, &msg, sizeof(struct init_msg), 0);
        if (recv_size < 0) {
            printf("Error: can't read init msg - %d\n", errno);
            continue;
        }

        // Выделение памяти под путь к файлу. Здесь +2 - дополнительные символы под "/" и "\0"
        char *filepath = malloc((strlen(save_dir) + strlen(msg.save_filename) + 2) * sizeof(char));
        sprintf(filepath, "%s/%s", save_dir, msg.save_filename);
        printf("Save path: %s\n", filepath);

        FILE  *file = fopen(filepath, "w");
        if (!file) {
            printf("Error: can't open file - %d\n", errno);
            continue;
        }

        unsigned int total_read_size = 0;
        char data_chunk[READ_DATA_CHUNK] = {0};

        while (total_read_size < msg.file_size) {
            size_t read_size = recv(conn_sock_d, data_chunk, READ_DATA_CHUNK, 0);
            if (read_size < 0) {
                printf("Error: can't read data - %d\n", errno);
                break;
            }

            int write_size = fwrite(data_chunk, read_size, 1, file);
            if (write_size < 0) {
                printf("Error: can't write to file - %d\n", errno);
                break;
            }
            total_read_size += read_size;
        }

        fclose(file);
        free(filepath);

        printf("From client: file name - %s, file size - %d, read - %d\n", msg.save_filename, msg.file_size, total_read_size);
    }

    return 0;
}