//
// Created by shenk on 04.04.2022.
//

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

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

    // Если пользователь не указал файл, который нужно отправить - выводим подсказку
    if (!filename) {
        printf("Usage: %s [-a address] [-p port] [-s save filename] filename\n", argv[0]);
        return 0;
    }

    // Если пользователь не указал сохраняемое имя файла, то используем обычное
    if (!save_filename) save_filename = filename;

    printf("File to send: %s\n", filename);
    printf("Save as: %s\n", save_filename);

    // Создание сокета. 0 - протокол по умолчанию для данного типа. Для SOCK_STREAM - TCP.
    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_d < 0) {
        printf("Error: can't create socket - %d\n", errno);
        return sock_d;
    }

    struct sockaddr_in sock_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(port)
    };

    // Преобразуем строку ip-адреса в число и сохраняем результат в sock_addr.sin_addr
    int is_addr_ok = inet_aton(addr, &sock_addr.sin_addr);
    if (!is_addr_ok) {
        printf("Error: wrong address - %d\n", errno);
        return 0;
    }

    // Подключение к серверу
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

    // Вызов sendfile, использующийся далее, использует дескриптор файла, а FILE*, так что получаем его.
    int file_d = fileno(file);

    // Получение размера файла
    struct stat buf;
    fstat(file_d, &buf);
    off_t size = buf.st_size;

    // Создание первого сообщения сервера
    struct init_msg msg = {
            .file_size = size
    };
    // Нельзя просто так преобразовать char* к char[MAX_FILENAME_LEN], так что копируем с проверкой на то, что не будет
    // скопировано больше, чем есть места
    strncpy(msg.save_filename, save_filename, MAX_FILENAME_LEN-1);
    // В случае, когда длина имени файла больше MAX_FILENAME_LEN, strncpy не поставит терминальный символ, так что
    // делаем вручную.
    msg.save_filename[MAX_FILENAME_LEN-1] = '\0';

    // Отправка первого сообщения
    ssize_t init_sent_size = send(sock_d, &msg, sizeof(struct init_msg), 0);
    if (init_sent_size < 0) {
        printf("Error: can't send init data - %d\n", errno);
        return init_sent_size;
    }

    // Отправляем файл с помощью sendfile. Благодаря этому можно избежать копирования файла из
    // kernel в user-space при чтении из него, а потом копирования из user в kernel-space при отправке данных
    // через send
    off_t offset = 0;
    ssize_t sent_size = sendfile(sock_d, file_d, &offset, size);
    printf("Sent %lu bytes\n", sent_size);

    fclose(file);
    close(sock_d);

    return 0;
}