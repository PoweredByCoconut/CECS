#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "server.h"

typedef struct {
    int fd;
    char name[MAX_CLIENT_NAME_LENGTH + 1];
} Client;

typedef struct {
    struct sockaddr_in socket_addr;
    struct sockaddr_in multicast_addr;
    char name[MAX_SERVER_NAME_LENGTH + 1];
    int socket;
    bool running;
    Client* clients;
    int num_clients;
    int max_clients;
} Server;

void start_server(Server *server, int port) {
    server->num_clients = 0;
    server->max_clients = 2;
    server->clients = malloc(server->max_clients * sizeof(Client));

    printf("What is the name of the server? ");
    fgets(server->name, MAX_SERVER_NAME_LENGTH, stdin);
    server->name[strcspn(server->name, "\n")] = '\0';

    memset(&(server->socket_addr), 0, sizeof(server->socket_addr));
    
    server->socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    server->socket_addr.sin_family = AF_INET;
    server->socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server->socket_addr.sin_port = htons(port);

    if (bind(server->socket, (struct sockaddr*)&server->socket_addr, sizeof(server->socket_addr)) < 0) {
        perror("bind");
        exit(-1);
    }

    if (listen(server->socket, 3) < 0) {
        perror("bind");
        exit(-1);
    }
}

int multicast(void* pserver) {
    Server* server = (Server*)pserver;

    int multicastfd = socket(AF_INET, SOCK_DGRAM, 0);

    char ttl = 3;
    setsockopt(multicastfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));


    while (server->running) {
        sendto(
                multicastfd,
                server->name,
                MAX_SERVER_NAME_LENGTH,
                0,
                (struct sockaddr*)&server->multicast_addr,
                sizeof(server->multicast_addr)
            );
        sleep(2);
    }
    
    close(multicastfd);

    return 0;
}

void send_to_clients(Server* server, const char* message, int origin) {
    for (int i = 0; i < server->num_clients; i++) {
        Client client = server->clients[i];
        
        if (client.fd != origin) {
            send(client.fd, message, strlen(message), 0);
        }
    }
}

thrd_t start_multicast(Server* server, const char* multi_addr, int port) {
    memset(&server->multicast_addr, 0, sizeof(server->multicast_addr));
    server->multicast_addr.sin_family = AF_INET;
    inet_pton(server->multicast_addr.sin_family, multi_addr, &(server->multicast_addr.sin_addr));
    server->multicast_addr.sin_port = htons(port);

    thrd_t multicast_thread;
    thrd_create(&multicast_thread, multicast, server);
    return multicast_thread;
}

int accept_thrd(void* pserver) {
    Server *server = (Server*)pserver;

    struct timespec sleep_time = {
        .tv_nsec = SLEEP_NANO,
    };

    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_length = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        int clientfd = accept(server->socket, (struct sockaddr*)&client_addr, &client_addr_length);

        if (clientfd >= 0) {
            if (server->num_clients < server->max_clients) {
                printf("accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));

                Client* client = &server->clients[server->num_clients++];               
                client->fd = clientfd;

                const char* name_prompt = "Welcome to the Server\nWhat is your name?";
                send(client->fd, name_prompt, strlen(name_prompt), 0);

                recv(client->fd, client->name, MAX_CLIENT_NAME_LENGTH, 0);
                client->name[MAX_CLIENT_NAME_LENGTH] = '\0';

                char welcome_message[MAX_MESSAGE_LENGTH] = {0};
                sprintf(welcome_message, "%s joined the server", client->name);
                send_to_clients(server, welcome_message, client->fd);
            }
            else {
                printf("Server full, rejecting connection from %s\n", inet_ntoa(client_addr.sin_addr));

                const char* message = "Server is full. Please try again later.";
                send(clientfd, message, strlen(message), 0);
                close(clientfd);
            }
        } else {
            thrd_sleep(&sleep_time, NULL);
        }
    }

    return 0;
}

thrd_t accept_connections(Server* server) {
    thrd_t accept_thread;
    thrd_create(&accept_thread, accept_thrd, server);
    return accept_thread;
}

static int recver(void* pserver) {
    Server* server = (Server*)pserver;

    char message_buffer[MAX_MESSAGE_LENGTH] = {0};

    struct timespec sleep_time = {
        .tv_nsec = SLEEP_NANO
    };

    while (server->running) {
        for (int i = 0; i < server->num_clients; i++) {
            Client client = server->clients[i];

            char message[MAX_MESSAGE_LENGTH + 1] = {0};
            ssize_t n = recv(client.fd, message, MAX_MESSAGE_LENGTH, MSG_DONTWAIT);

            if (n > 0) {
                message[n] = '\0';
                
                if (strcmp(message, "exit") == 0) {
                    snprintf(message_buffer, MAX_MESSAGE_LENGTH, "%s left the chat", client.name);

                    close(client.fd);
                    printf("disconnected %s\n", client.name);

                    server->num_clients--;
                    for (int j = i; j < server->num_clients; j++) {
                        server->clients[j] = server->clients[j + 1];
                    }
                } else {
                    snprintf(message_buffer, MAX_MESSAGE_LENGTH, "%s: %s", client.name, message);
                }

                send_to_clients(server, message_buffer, client.fd);
            } else {
                thrd_sleep(&sleep_time, NULL);
            }
        }
    }

    return 0;
}

thrd_t create_recver(Server* server) {
    thrd_t recver_thrd;
    thrd_create(&recver_thrd, recver, server);
    return recver_thrd;
}

int run_server(void) {
    Server server = {0};

    start_server(&server, MESSAGE_PORT);

    server.running = true;

    thrd_t multicast_thread = start_multicast(&server, MULTI_ADDR, MULTI_PORT);
    thrd_t accept_thread = accept_connections(&server);
    thrd_t recver_thrd = create_recver(&server);

    char command[32] = {0};
    while (strncmp(command, "exit", 31)) {
        printf("type exit to stop\n");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';
    }
    server.running = false;
    printf("Stopping server\n");

    thrd_join(multicast_thread, NULL);
    thrd_join(accept_thread, NULL);
    thrd_join(recver_thrd, NULL);
    close(server.socket);

    for (int i = 0; i < server.num_clients; i++) {
        close(server.clients[i].fd);
    }

    return 0;
}
