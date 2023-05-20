#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345 // порт для подключения клиентов

struct client {
    int socket;
    char name[256];
};

struct client *clients = NULL;
int num_clients = 0;

void *handle_client(void *arg) {
    struct client *client_data = (struct client*)arg;
    int client_socket = client_data->socket;
    char name[256];
    char buffer[256];
    char message[1024];
    int bytes_read;
    int i;

    // Получаем имя клиента
    bytes_read = read(client_socket, name, sizeof(name));
    name[bytes_read] = '\0';
    strcpy(client_data->name, name);

    // Отправляем приветственное сообщение клиенту
    snprintf(message, sizeof(message), "Добро пожаловать, %s! Вы подключены к серверу чата\n", name);
    write(client_socket, message, strlen(message));

    // Отправляем сообщение о новом клиенте всем остальным клиентам
    snprintf(message, sizeof(message), "Клиент %s подключился к чату\n", name);
    for (i = 0; i < num_clients; i++) {
        if (clients[i].socket != client_socket) {
            write(clients[i].socket, message, strlen(message));
        }
    }

    // Добавляем клиента в список
    num_clients++;
    clients = (struct client*)realloc(clients, num_clients * sizeof(struct client));
    clients[num_clients-1].socket = client_socket;
    strcpy(clients[num_clients-1].name, name);

    // Обрабатываем сообщения от клиента
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) {
        // Удаляем символ новой строки из конца сообщения
        if (buffer[bytes_read-1] == '\n') {
            buffer[bytes_read-1] = '\0';
            bytes_read--;
        }

        // Формируем сообщение для отправки всем клиентам
        snprintf(message, sizeof(message), "%s: %s\n", name, buffer);

        // Отправляем сообщение всем клиентам
        for (i = 0; i < num_clients; i++) {
            if (clients[i].socket != client_socket) {
                write(clients[i].socket, message, strlen(message));
            }
        }

        // Выводим сообщение в консоль сервера
        printf("%s", message);
    }

    // Отправляем сообщение об отключении клиента всем остальным клиентам
    snprintf(message, sizeof(message), "Клиент %s отключился от чата\n", name);
    for (i = 0; i < num_clients; i++) {
        if (clients[i].socket != client_socket) {
            write(clients[i].socket, message, strlen(message));
        }
    }

    // Удаляем клиента из списка
    for (i = 0; i < num_clients; i++) {
        if (clients[i].socket == client_socket) {
            clients[i].socket = -1;
            break;
        }
    }
    for (i = i+1; i < num_clients; i++) {
        clients[i-1] = clients[i];
    }
    num_clients--;
    clients = (struct client*)realloc(clients, num_clients * sizeof(struct client));

    // Закрываем сокет клиента
    close(client_socket);

    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address; 
    pthread_t threads;
    socklen_t client_address_len = sizeof(client_address);

    // Создаем динамический массив для хранения клиентов
    clients = (struct client*)malloc(0);

    // Создаем сокет сервера
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Ошибка создания сокета сервера");
        exit(EXIT_FAILURE);
    }

    // Заполняем адрес сервера
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Привязываем сокет кадресу сервера
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Ошибка привязки сокета к адресу сервера");
        exit(EXIT_FAILURE);
    }

    // Начинаем прослушивание сокета
    if (listen(server_socket, 10) == -1) {
        perror("Ошибка начала прослушивания сокета");
        exit(EXIT_FAILURE);
    }

    // Обрабатываем подключение новых клиентов
    while (1) {
        // Принимаем новое подключение
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Ошибка принятия нового подключения");
            continue;
        }

        // Создаем новый поток для обработки клиента
        struct client new_client;
        new_client.socket = client_socket;
        pthread_create(&threads, NULL, handle_client, (void*)&new_client);
    }

    return 0;
}