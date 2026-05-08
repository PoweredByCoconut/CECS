#include <asm-generic/socket.h>
#define _GNU_SOURCE

#include "client.h"
#include "server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <threads.h>
#include <time.h>
#include "cecs.h"

typedef struct {
    bool running;
    int host_socket;
} Client;

struct in_addr find_host(const char* multi_addr, int multi_port) {
    int multifd = socket(AF_INET, SOCK_DGRAM, 0);

    int optval = 1;
    setsockopt(multifd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct sockaddr_in host_addr, in_addr;
    memset(&host_addr, 0, sizeof(host_addr));
    socklen_t host_addr_length = sizeof(host_addr);

    in_addr.sin_family = AF_INET;
    inet_pton(in_addr.sin_family, multi_addr, &(in_addr.sin_addr));
    in_addr.sin_port = htons(multi_port);

    if (bind(multifd, (struct sockaddr*)&in_addr, sizeof(in_addr)) < 0) {
        printf("Error binding socket\n");
        perror("bind");
        return (struct in_addr) {0};
    }

    struct ip_mreqn group;
    group.imr_multiaddr.s_addr = inet_addr(multi_addr);
    group.imr_address.s_addr = INADDR_ANY;
    group.imr_ifindex = 0;

    setsockopt(multifd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));

    int max_message_length = 128;
    char message[max_message_length + 1];

    char connect = '\0';

    while (connect != 'y' && connect != 'Y') {
        ssize_t length = recvfrom(multifd, message, max_message_length, 0, (struct sockaddr*)&host_addr, &host_addr_length);
        message[length] = '\0';

        char address[16];
        inet_ntop(host_addr.sin_family, &(host_addr.sin_addr), address, sizeof(address));

        printf("%s sent: %s\ndo you want to connect [y/N]?\n", address, message);

        connect = get_char_clear();
    }

    close(multifd);

    return host_addr.sin_addr;
}

int connect_host(struct in_addr host_address, int port) {
    struct sockaddr_in host_addr;
    memset(&host_addr, 0, sizeof(host_addr));
    int host_socket = socket(AF_INET, SOCK_STREAM, 0);

    host_addr.sin_family = AF_INET;
    host_addr.sin_addr = host_address;
    host_addr.sin_port = htons(port);

    if (connect(host_socket, (struct sockaddr*)&host_addr, sizeof(host_addr)) != 0) {
        fprintf(stderr, "Could not connect to server\n");
        perror("connect");
        exit(2);
    }

    return host_socket;
}

int sender(void* pclient) {
    Client *client = (Client*)pclient;

    char message_buffer[MAX_MESSAGE_LENGTH] = {'\0'};

    while (client->running) {
        fgets(message_buffer, MAX_MESSAGE_LENGTH, stdin);
        message_buffer[strcspn(message_buffer, "\n")] = '\0';
        send(client->host_socket, message_buffer, strlen(message_buffer), 0);
        client->running = strcmp(message_buffer, "exit") != 0;
    }

    return 0;
}

static int recver(void* pclient) {
    Client *client = (Client*)pclient;

    char message_buffer[MAX_MESSAGE_LENGTH] = {0};

    struct timespec sleep_time = {
        .tv_nsec = SLEEP_NANO
    };

    while (client->running) {
        ssize_t n = recv(client->host_socket, message_buffer, MAX_MESSAGE_LENGTH, MSG_DONTWAIT);
        if (n > 0) {
            message_buffer[n] = '\0';
            printf("%s\n", message_buffer);
        } else {
            thrd_sleep(&sleep_time, NULL);
        }
    }

    return 0;
}

void start_communication(Client* client) {
    thrd_t send_thrd, recv_thrd;
    client->running = true;

    thrd_create(&send_thrd, sender, client);
    thrd_create(&recv_thrd, recver, client);

    thrd_join(send_thrd, NULL);
    thrd_join(recv_thrd, NULL);
}

int start_client(void) {
    Client client;

    struct in_addr host_address = find_host(MULTI_ADDR, MULTI_PORT);
    client.host_socket = connect_host(host_address, MESSAGE_PORT);

    printf("connected\n");

    start_communication(&client);

    close(client.host_socket);

    return 0;
}
