#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080

void *handle_client(void *arg);

int main() {
    // printf("THIS IS NEW VERSION\n");
    // fflush(stdout);
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failure");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &addr_len);

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;

        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    // printf("New client thread started\n");
    // fflush(stdout);

    char buffer[256];

    while (1) {
        // printf("Waiting for data...\n");
        // fflush(stdout);


        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        // printf("recv returned: %d\n", n);
        // fflush(stdout);

        if (n <= 0)
            break;

        buffer[n] = '\0';
        // printf("Client says: %s\n", buffer);
        // fflush(stdout);

        write(STDOUT_FILENO, "Client says: ", 13);
        write(STDOUT_FILENO, buffer, n);
        write(STDOUT_FILENO, "\n", 1);

        send(client_fd, buffer, n, 0);
    }

    close(client_fd);
    return NULL;
}
