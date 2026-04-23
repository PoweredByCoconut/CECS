#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    bool success;
    int socket;
} Connection;

typedef struct {
    bool ok;
    union {
        int int_val;
    };
} Result;

void unwrap(Result result, const char* message) {
    if (!result.ok) {
        fprintf(stderr, "Failed to unwrap result:\n%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int unwrap_int(Result result, const char* message) {
    if (!result.ok) {
        fprintf(stderr, "Failed to unwrap result:\n%s\n", message);
        exit(EXIT_FAILURE);
    }
    return result.int_val;
}

Result create_socket(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    return (Result) {
        .ok = sockfd >= 0,
        .int_val = sockfd,
    };
}

int bind_socket_to_port(int sockfd, unsigned short port) {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    int ret = bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (ret != 0) {
        fprintf(stderr, "failed to bind socket\n");
        perror("bind");
    }

    return ret;
}

int try_host(unsigned short port) {
    int sockfd = unwrap_int(create_socket(), "Failed to create socket");

    if (bind_socket_to_port(sockfd, port) != 0) {
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 5) != 0){
        fprintf(stderr, "failed to listen to socket\n");
        close(sockfd);
        return -1;
    }

    printf("hosting\n");

    struct sockaddr_in client_address;
    socklen_t client_size;
    int client_socket = accept(sockfd, (struct sockaddr*)&client_address, &client_size);

    printf("accepted with descriptor %d\n", client_socket);

    send(client_socket, "test", 5, 0);

    return sockfd;
}

int try_connect(unsigned short port) {
    int sockfd = unwrap_int(create_socket(), "Failed to create socket");

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        fprintf(stderr, "Failed to connect to socket\n");
        perror("connect");
        close(sockfd);
        return -1;
    }

    printf("Connected as client\n");

    char buf[5];
    recv(sockfd, buf, sizeof(buf), 0);

    printf("Message received: %.4s\n", buf);

    return sockfd;
}

int host_or_connect(unsigned short port, bool* hosting) {
    int sockfd = try_host(port);
    if (sockfd < 0) {
        sockfd = try_connect(port);
        *hosting = false;
    } else {
        *hosting = true;
    }

    return sockfd;
}

int main(void) {
    bool hosting;

    int sockfd = host_or_connect(5498, &hosting);

    char buf[128] = {0};

    if (hosting) {
        scanf("%127s", buf);
        send(sockfd, buf, sizeof(buf), 0);
    }

    while (strncmp(buf, "exit", 4) != 0) {
        recv(sockfd, buf, sizeof(buf), 0);
        printf("%s\n", buf);
        scanf("%127s", buf);
        send(sockfd, buf, sizeof(buf), 0);
    }

    close(sockfd);
}
