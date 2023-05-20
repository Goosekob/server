#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1" // IP адрес сервера
#define PORT 12345 // порт для подключения клиентов

void *read_messages(void *arg) {
    int client_socket = *(int*)arg;
    char buffer[256];
    int bytes_read;

    while (1) {
        bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }

    pthread_exit(NULL);
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char name[256];
    char buffer[256];
    int exit_requested = 0;

    // Создаем сокет клиента
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Ошибка создания сокета клиента");
        exit(EXIT_FAILURE);
    }

    // Заполняем адрес сервера
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_address.sin_port = htons(PORT);

    // Устанавливаем соединение с сервером
    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        perror("Ошибка подключения к серверу");
        exit(EXIT_FAILURE);
    }

    // Получаем имя пользователя
    printf("Введите ваше имя: ");
    fgets(name, sizeof(name), stdin);
    name[strlen(name)-1] = '\0';

    // Отправляем имя пользователя на сервер
    write(client_socket, name, strlen(name));

    // Получаем приветственное сообщение от сервера
    int bytes_read = read(client_socket, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    printf("%s", buffer);

    // Создаем новый поток для чтения сообщений от сервера
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, read_messages, &client_socket) != 0) {
        perror("Ошибка создания потока чтения сообщений");
        exit(EXIT_FAILURE);
    }

    // Цикл ввода сообщений и их отправки на сервер
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }
        write(client_socket, buffer, strlen(buffer));
    }

    // Закрываем сокет клиента
    close(client_socket);

    return 0;
}
