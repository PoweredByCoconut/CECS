#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ifaddrs.h>
#include <netdb.h>

#define MAX_MSG_LENGTH 128

typedef struct {
    int sockfd;
    struct sockaddr_in sockaddr;
} multicast_data;

bool running;

int sender(void* psockfd) {
    int sockfd = *(int*)psockfd;

    char message[MAX_MSG_LENGTH + 1];
    message[0] = '\0';

    printf("Enter a message\n");

    while (running) {
        fgets(message, MAX_MSG_LENGTH, stdin);
        message[strcspn(message, "\n")] = '\0';
        send(sockfd, message, strlen(message), 0);
        running = strcmp(message, "exit") != 0;
    }

    printf("stopping\n");

    return 0;
}

int recver(void* psockfd) {
    int sockfd = *(int*)psockfd;

    char message[MAX_MSG_LENGTH + 1];
    message[0] = '\0';

    printf("Waiting for message\n");

    struct timespec sleep_time = {
        .tv_nsec = 1000
    };

    while (running) {
        ssize_t n = recv(sockfd, message, MAX_MSG_LENGTH, MSG_DONTWAIT);
        if (n > 0) {
            message[n] = '\0';
            printf("%s\n", message);
        } else {
            thrd_sleep(&sleep_time, NULL);
        }
    }

    return 0;
}

int multicast(void* pdata) {
    multicast_data data = *(multicast_data*)pdata;
    const char* message = "I'm a little server";
    size_t message_length = strlen(message);
    
    struct timespec sleep_time = {
        .tv_sec = 2,
    };

    running = true;

    while (running) {
        sendto(data.sockfd, message, message_length, 0, (struct sockaddr*)&data.sockaddr, sizeof(data.sockaddr));
        thrd_sleep(&sleep_time, NULL);
    }

    close(data.sockfd);
    free(pdata);

    return 0;
}

int try_host(int port) {
    struct sockaddr_in server = {0};
    int server_socket ;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        return -1;
    }

    if (listen(server_socket, 1) < 0) {
        return -1;
    }

    return server_socket;
}

int try_connect(struct in_addr host_address, int port) {
    struct sockaddr_in client = {0};
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    client.sin_family = AF_INET;
    client.sin_addr = host_address;
    client.sin_port = htons(port);

    if (connect(server_socket, (struct sockaddr*)&client, sizeof(client)) != 0) {
        fprintf(stderr, "Failed to connect to server\n");
        perror("connect");
        return -1;
    }

    return server_socket;
}

char get_char_clear(void) {
    int character = getchar();

    int temp;
    if (character != '\n' && character != EOF) {
        while ((temp = getchar()) != '\n' && temp != EOF);
    }

    return character;
}

thrd_t broadcast_host(void) {
    int broadcastfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("239.0.0.1");
    dest_addr.sin_port = htons(6841);

    char ttl = 3;
    setsockopt(broadcastfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    multicast_data* pdata = malloc(sizeof(multicast_data));
    pdata->sockfd = broadcastfd;
    pdata->sockaddr = dest_addr;
    thrd_t broadcast_thread;
    thrd_create(&broadcast_thread, multicast, pdata);

    return broadcast_thread;
}

struct in_addr find_host(void) {
    int multifd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in host_addr = {0}, in_addr;
    socklen_t host_addr_length = sizeof(host_addr);

    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    in_addr.sin_port = htons(6841);

    if (bind(multifd, (struct sockaddr*)&in_addr, sizeof(in_addr)) < 0) {
        printf("Error binding socket\n");
        perror("bind");
        return (struct in_addr) {0};
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.0.0.1");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    setsockopt(multifd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    int max_message_length = 128;
    char message[max_message_length + 1];

    char connect = '\0';

    while (connect != 'y' && connect != 'Y') {
        ssize_t length = recvfrom(multifd, message, max_message_length, 0, (struct sockaddr*)&host_addr, &host_addr_length);
        message[length] = '\0';

        printf("%s sent: %s\n", inet_ntoa(host_addr.sin_addr), message);

        printf("do you want to connect [y/N]? ");
        connect = get_char_clear();
    }

    close(multifd);

    return host_addr.sin_addr;
}

void start_comms(int sockfd) {
    thrd_t send_thrd, recv_thrd;
    running = true;
    thrd_create(&send_thrd, sender, &sockfd);
    thrd_create(&recv_thrd, recver, &sockfd);

    thrd_join(send_thrd, NULL);
    thrd_join(recv_thrd, NULL);
}

int main(void) {
    printf("Do you want to host [y/N]? ");
    char host = get_char_clear();

    if (host == 'y' || host == 'Y') {
        int sockfd = try_host(4596);

        thrd_t broadcast_thread = broadcast_host();

        struct sockaddr_in client_addr = {0};
        socklen_t client_addr_length = sizeof(client_addr);

        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_length);

        printf("accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));

        start_comms(clientfd);

        thrd_join(broadcast_thread, NULL);

        close(sockfd);

        close(clientfd);
    } else {
        printf("looking for hosts\n");

        struct in_addr host_address = find_host();
        int sockfd = try_connect(host_address, 4596);

        start_comms(sockfd);

        close(sockfd);
    }
}
