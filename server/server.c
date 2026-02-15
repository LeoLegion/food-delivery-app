#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080

void *handle_client(void *arg);

#define MAX_USERS 100

typedef struct {
    char username[50];
    char password[50];
} User;

User users[MAX_USERS];
int user_count = 0;

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;

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

int register_user(const char *username, const char *password) {
    pthread_mutex_lock(&user_mutex);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&user_mutex);
            return 0;
        }
    }

    if (user_count >= MAX_USERS) {
        pthread_mutex_unlock(&user_mutex);
        return -1;
    }

    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    user_count++;

    pthread_mutex_unlock(&user_mutex);
    return 1;
}

int login_user(const char *username, const char *password) {
    pthread_mutex_lock(&user_mutex);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) ==0) {
            pthread_mutex_unlock(&user_mutex);
            return 1;
        }
    }

    pthread_mutex_unlock(&user_mutex);
    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    // printf("New client thread started\n");
    // fflush(stdout);

    char buffer[256];

    int is_logged_in = 0;
    char current_user[50];

    while (1) {
        // printf("Waiting for data...\n");
        // fflush(stdout);


        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        // printf("recv returned: %d\n", n);
        // fflush(stdout);

        if (n <= 0)
            break;

        buffer[n] = '\0';
        printf("Client says: %s\n", buffer);
        // fflush(stdout);

        // write(STDOUT_FILENO, "Client says: ", 13);
        // write(STDOUT_FILENO, buffer, n);
        // write(STDOUT_FILENO, "\n", 1);

        // send(client_fd, buffer, n, 0);

        char command[20], username[50], password[50];

        int args = sscanf(buffer, "%s %s %s", command, username, password);
        
        if (strcmp(command, "REGISTER") == 0) {

            if (args != 3) {
                send(client_fd, "INVALID_FORMAR\n", 15, 0);
                continue;
            }

            int result = register_user(username, password);

            if (result == 1) {
                send(client_fd, "REGISTER_SUCCESS\n", 17, 0);
            } else if (result == 0) {
                send(client_fd, "USER_ALREADY_EXISTS\n", 20, 0);
            } else {
                send(client_fd, "SERVER_FULL\n", 12, 0);
            }
        }
        
        else if (strcmp(command, "LOGIN") == 0) {

            if (args != 3) {
                send(client_fd, "INVALID_FORMAT\n", 15, 0);
                continue;
            }

            if (login_user(username, password)) {
                is_logged_in = 1;
                strcpy(current_user, username);
                send(client_fd, "LOGIN_SUCCESS\n", 14, 0);
            } else {
                send(client_fd, "LOGIN_FAILED\n", 13, 0);
            }
        }
        
        else {
            send(client_fd, "INVALID_COMMAND\n", 16, 0);
        }
            
        }
    

    close(client_fd);
    return NULL;
}
