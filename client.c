#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345

void *read_messages(void *arg) 
{
    int client_socket = *(int*)arg;
    char buffer[256];
    int bytes_read;

    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) 
    {
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

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("Ошибка создания сокета клиента");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_address.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) 
    {
        perror("Ошибка подключения к серверу");
        exit(EXIT_FAILURE);
    }

    printf("Введите ваше имя: ");
    fgets(name, sizeof(name), stdin);
    name[strlen(name)-1] = '\0';

    write(client_socket, name, strlen(name));

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, read_messages, &client_socket) != 0) 
    {
        perror("Ошибка создания потока чтения сообщений");
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), stdin)) 
    {
        if (strcmp(buffer, "exit\n") == 0) 
        {
            break;
        }
        write(client_socket, buffer, strlen(buffer));
    }

    close(client_socket);

    return 0;
}