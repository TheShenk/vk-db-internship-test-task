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
#include <unistd.h>

#include "common_structs.h"

#define READ_DATA_CHUNK 1023

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

    // В случае, если папки не существует, то пробуем создать
    struct stat dir = {0};
    // Проверку на существование - попытка получение информации о папке
    if(stat(save_dir, &dir) == -1) {
        int mkdir_err = mkdir(save_dir, 0755);
        if (mkdir_err < 0) {
            printf("Error: can't create directory - %d\n", errno);
            return mkdir_err;
        } else {
            printf("Directory created\n");
        }
    }

    // Создание сокета. 0 - протокол по умолчанию для данного типа. Для SOCK_STREAM - TCP.
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

    // Привязка сокета к порту
    int err = bind(sock_d, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        printf("Error: can't bind socket - %d\n", errno);
        close(sock_d);
        return err;
    }

    // Так как TCP протокол использует соединения, то начинаем слушать его. 1 - число одновременных соединений.
    err = listen(sock_d, 1);
    if (err < 0) {
        printf("Error: can't listen - %d\n", errno);
        close(sock_d);
        return err;
    }

    int val = 1;
    setsockopt(sock_d, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Бесконечный цикл - ожидаем новых файлов
    while (1) {
        // Принимаем соединение, accept возвращает дескриптор нового сокета, который нужно использовать для
        // приема/отправки данных
        int conn_sock_d = accept(sock_d, NULL, NULL);
        if (conn_sock_d < 0) {
            printf("Error: can't accept - %d\n", errno);
            continue;
        }

        // Ожидаем первого сообщения, которое передаст имя и размер файла
        struct init_msg msg;
        ssize_t recv_size = recv(conn_sock_d, &msg, sizeof(struct init_msg), 0);
        if (recv_size < 0) {
            printf("Error: can't read init msg - %d\n", errno);
            close(conn_sock_d);
            continue;
        }

        // Выделение памяти под путь к файлу. Здесь +2 - дополнительные символы под "/" и "\0"
        char *filepath = malloc((strlen(save_dir) + strlen(msg.save_filename) + 2) * sizeof(char));
        if (!filepath) {
            printf("Error: can't malloc\n");
            close(conn_sock_d);
            continue;
        }
        sprintf(filepath, "%s/%s", save_dir, msg.save_filename);
        printf("Save path: %s\n", filepath);

        FILE  *file = fopen(filepath, "w");
        if (!file) {
            printf("Error: can't open file - %d\n", errno);
            close(conn_sock_d);
            free(filepath);
            continue;
        }

        unsigned long long total_read_size = 0;
        char data_chunk[READ_DATA_CHUNK] = {0};

        // Читаем данные из сокета, до тех пор, пока количество прочитанных байт меньше размера файла
        while (total_read_size < msg.file_size) {
            size_t read_size = recv(conn_sock_d, data_chunk, READ_DATA_CHUNK, 0);
            if (read_size < 0) {
                printf("Error: can't read data - %d\n", errno);
                fclose(file);
                free(filepath);
                close(conn_sock_d);
                continue;
            }

            // Записываем столько байт, сколько прочитали
            int write_size = fwrite(data_chunk, read_size, 1, file);
            if (write_size < 0) {
                printf("Error: can't write to file - %d\n", errno);
                fclose(file);
                free(filepath);
                close(conn_sock_d);
                continue;
            }

            // Запоминаем общее число прочитанных байт
            total_read_size += read_size;
            printf("Received %lu bytes, %.1f%% of file\n", read_size, (float)total_read_size/msg.file_size*100);
        }

        printf("From client: file name - %s, file size - %llu, read - %llu\n", msg.save_filename, msg.file_size, total_read_size);

        fclose(file);
        free(filepath);
        close(conn_sock_d);
    }

    return 0;
}