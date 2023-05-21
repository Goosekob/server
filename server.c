#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"

struct client 
{
    int socket;
    char name[256];
};

struct client *clients = NULL;
int num_clients = 0;


void list_members(int client_socket) 
{
    char message[1024];
    int i;

    snprintf(message, sizeof(message), "Список активных участников чата:\n");
    write(client_socket, message, strlen(message));

    for (i = 0; i < num_clients; i++) 
    {
        snprintf(message, sizeof(message), "%s\n", clients[i].name);
        write(client_socket, message, strlen(message));
    }
}

void *handle_client(void *arg) 
{
    struct client *client_data = (struct client*)arg;
    int client_socket = client_data->socket;
    char name[256];
    char buffer[256];
    char message[1024];
    int bytes;
    int i;

    bytes = read(client_socket, name, sizeof(name));
    name[bytes] = '\0';
    strcpy(client_data->name, name);

    snprintf(message, sizeof(message), "Добро пожаловать в самый безопасный чат, %s!\n", name);
    write(client_socket, message, strlen(message));

    snprintf(message, sizeof(message), "Клиент %s зашёл в чат\n", name);
    for (i = 0; i < num_clients; i++) 
    {
        if (clients[i].socket != client_socket) 
        {
            write(clients[i].socket, message, strlen(message));
        }
    }

    num_clients++;
    struct client *new_clients = (struct client*)realloc(clients, num_clients * sizeof(struct client));
    if (new_clients == NULL) {} 
    else 
    {
        clients = new_clients;
    }    
    clients[num_clients-1].socket = client_socket;
    strcpy(clients[num_clients-1].name, name);

while ((bytes = read(client_socket, buffer, sizeof(buffer))) > 0) 
{
    if (buffer[bytes-1] == '\n') 
    {
        buffer[bytes-1] = '\0';
        bytes--;
    }

    if (strcmp(buffer, "/members") == 0) 
    {
        list_members(client_socket);
    } else 
    {
        snprintf(message, sizeof(message), "%s: %s\n", name, buffer);

        for (i = 0; i < num_clients; i++) 
        {
            if (clients[i].socket != client_socket) 
            {
                write(clients[i].socket, message, strlen(message));
            }
        }

        printf("%s", message);
    }
}

    snprintf(message, sizeof(message), "Клиент %s вышел из чата\n", name);

    for (i = 0; i < num_clients; i++) 
    {
        if (clients[i].socket != client_socket) 
        {
            write(clients[i].socket, message, strlen(message));
        }
    }

    for (i = 0; i < num_clients; i++) 
    {
        if (clients[i].socket == client_socket) 
        {
            clients[i].socket = -1;
            break;
        }
    }

    for (i = i+1; i < num_clients; i++) 
    {
        clients[i-1] = clients[i];
    }
    num_clients--;
    clients = (struct client*)realloc(clients, num_clients * sizeof(struct client));

    close(client_socket);

    return NULL;
}

int main() 
{
    int server_socket;
    struct sockaddr_in address;
    struct sockaddr_in client_address; 
    pthread_t threads;
    socklen_t client_address_len = sizeof(client_address);

    clients = (struct client*)malloc(0);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("Ошибка создания сокета сервера");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP);;
    address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) == -1) 
    {
        perror("Ошибка привязки сокета к адресу сервера");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) 
    {
        perror("Ошибка начала прослушивания сокета");
        exit(EXIT_FAILURE);
    }

    printf("ФСБ теперь прослушивает сервер %s, порт %d\n", SERVER_IP, PORT);

    while (1) 
    {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) 
        {
            perror("Ошибка принятия нового подключения");
            continue;
        }

        struct client new_client;
        new_client.socket = client_socket;
        pthread_create(&threads, NULL, handle_client, (void*)&new_client);
    }

    return 0;
}