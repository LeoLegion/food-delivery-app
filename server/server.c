#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080

void *handle_client(void *arg);
void load_users();
void load_restaurants();

#define MAX_USERS 100

typedef struct {
    char username[50];
    char password[50];
} User;

User users[MAX_USERS];
int user_count = 0;

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_RESTAURANTS 50
#define MAX_MENU_ITEMS 20

typedef struct {
    char name[100];
    char menu[MAX_MENU_ITEMS][100];
    int menu_count;
} Restaurant;

Restaurant restaurants[MAX_RESTAURANTS];
int restaurant_count = 0;

pthread_mutex_t restaurant_mutex = PTHREAD_MUTEX_INITIALIZER;

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

    load_users();
    load_restaurants();

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

void save_users() {
    FILE *fp = fopen("users.txt", "w");
    if (!fp) return;

    for (int i = 0; i < user_count; i++) {
        fprintf(fp, "%s %s", users[i].username, users[i].password);
    }

    fclose(fp);
}

void load_users() {
    FILE *fp = fopen("users.txt", "r");
    if (!fp) return;

    while (fscanf(fp, "%s %s", users[user_count].username, users[user_count].password) == 2) {
        user_count++;
    }

    fclose(fp);
}

void save_restaurants() {
    FILE *fp = fopen("restaurants.txt", "w");
    if (!fp) return;

    for (int i = 0; i < restaurant_count; i++) {
        fprintf(fp, "%s %d\n", restaurants[i].name, restaurants[i].menu_count);

        for (int j = 0; j < restaurants[i].menu_count; j++) {
            fprintf(fp, "%s\n", restaurants[i].menu[j]);
        }
    }

    fclose(fp);
}

void load_restaurants() {
    FILE *fp = fopen("restaurants.txt", "r");
    if (!fp) return;

    while (fscanf(fp, "%s %d", restaurants[restaurant_count].name, &restaurants[restaurant_count].menu_count) == 2) {
        for (int j = 0; j < restaurants[restaurant_count].menu_count; j++) {
            fscanf(fp, "%s", restaurants[restaurant_count].menu[j]);
        }

        restaurant_count++;
    }

    fclose(fp);
}

int add_restaurant(const char *name) {
    pthread_mutex_lock(&restaurant_mutex);

    if (restaurant_count >= MAX_RESTAURANTS) {
        pthread_mutex_unlock(&restaurant_mutex);
        return 0;
    }

    strcpy(restaurants[restaurant_count].name, name);
    restaurant_count++;
    save_restaurants();

    pthread_mutex_unlock(&restaurant_mutex);
    return 1;
}

int find_restaurant(const char *name) {
    for (int i = 0; i < restaurant_count; i++) {
        if (strcmp(restaurants[i].name, name) == 0)
            return i;
    }
    return -1;
}

int add_menu_item(const char *restaurant, const char *item) {
    
    pthread_mutex_lock(&restaurant_mutex);

    int index = find_restaurant(restaurant);
    
    if (index == -1) {
        pthread_mutex_unlock(&restaurant_mutex);
        return -1;
    }

    if (restaurants[index].menu_count >= MAX_MENU_ITEMS) {
        pthread_mutex_unlock(&restaurant_mutex);
        return 0;
    }

    strcpy(restaurants[index].menu[restaurants[index].menu_count], item);
    restaurants[index].menu_count++;
    save_restaurants();

    pthread_mutex_unlock(&restaurant_mutex);
    return 1;
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
    save_users();

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

        // char command[20], username[50], password[50];
        char command[20];
        char arg1[100];
        char arg2[100];

        int args = sscanf(buffer, "%s %s %s", command, arg1, arg2);
        // int args = sscanf(buffer, "%s %s %s", command, username, password);
        
        if (strcmp(command, "REGISTER") == 0) {

            if (args != 3) {
                send(client_fd, "INVALID_FORMAT\n", 15, 0);
                continue;
            }

            int result = register_user(arg1, arg2);

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

            if (login_user(arg1, arg2)) {
                is_logged_in = 1;
                strcpy(current_user, arg1);
                send(client_fd, "LOGIN_SUCCESS\n", 14, 0);
            } else {
                send(client_fd, "LOGIN_FAILED\n", 13, 0);
            }
        }
        
        else if (strcmp(command, "ADD_RESTAURANT") == 0) {

            if (!is_logged_in) {
                send(client_fd, "LOGIN_REQUIRED\n", 15, 0);
                continue;
            }

            if (args < 2) {
                send(client_fd, "INVALID_FORMAT\n", 15, 0);
                continue;
            }

            if (add_restaurant(arg1))
                send(client_fd, "RESTAURANT_ADDED\n", 17, 0);
            else
                send(client_fd, "RESTAURANT_LIMIT_REACHED\n", 24, 0);
        }

        else if (strcmp(command, "LIST_RESTAURANTS") == 0) {
            
            if (!is_logged_in) {
                send(client_fd, "LOGIN_REQUIRED\n", 15, 0);
                continue;
            }

            pthread_mutex_lock(&restaurant_mutex);

            if (restaurant_count == 0) {
                send(client_fd, "NO_RESTAURANTS\n", 15, 0);
            } else {
                char response[1024] = "";
                for (int i = 0; i < restaurant_count; i++) {
                    strcat(response, restaurants[i].name);
                    strcat(response, "\n");
                }
                send(client_fd, response, strlen(response), 0);
            }

            pthread_mutex_unlock(&restaurant_mutex);
        } 
        
        else if (strcmp(command, "ADD_MENU") == 0) {
            if (!is_logged_in) {
                send(client_fd, "LOGIN_REQUIRED\n", 15, 0);
                continue;
            }

            if (args < 3) {
                send(client_fd, "INVALID_FORMAT\n", 15, 0);
                continue;
            }

            int result = add_menu_item(arg1, arg2);

            if (result == 1)
                send(client_fd, "MENU_ADDED\n", 11, 0);
            else if (result == 0)
                send(client_fd, "MENU_FULL\n", 10, 0);
            else
                send(client_fd, "RESTAURANT_NOT_FOUND\n", 21, 0);
        }

        else if (strcmp(command, "LIST_MENU") == 0) {
            if (!is_logged_in) {
                send(client_fd, "LOGIN_REQUIRED\n", 15, 0);
                continue;
            }

            pthread_mutex_lock(&restaurant_mutex);

            int index = find_restaurant(arg1);

            if (index == -1) {
                send(client_fd, "RESTAURANT_NOT_FOUND\n", 21, 0);
            } else if (restaurants[index].menu_count == 0) {
                send(client_fd, "MENU_EMPTY\n", 11, 0);
            } else {
                char response[1024] = "";
                for (int i = 0; i < restaurants[index].menu_count; i++) {
                    strcat(response, restaurants[index].menu[i]);
                    strcat(response, "\n");
                }
                send(client_fd, response, strlen(response), 0);
            }

            pthread_mutex_unlock(&restaurant_mutex);
        }

        else if (strcmp(command, "PLACE_ORDER") == 0) {

            if (!is_logged_in) {
                send(client_fd, "LOGIN_REQUIRED\n", 15, 0);
                continue;
            }

            pthread_mutex_lock(&restaurant_mutex);
            
            int index = find_restaurant(arg1);

            if (index == -1) {
                send(client_fd, "RESTAURANT_NOT_FOUND\n", 21, 0);
            } else {
                int found = 0;
                for (int i = 0; i < restaurants[index].menu_count; i++) {
                    if (strcmp(restaurants[index].menu[i], arg2) == 0) {
                        found = 1;
                        break;
                    }
                }

                if (found)
                    send(client_fd, "ORDER_PLACED\n", 13, 0);
                else
                    send(client_fd, "ITEM_NOT_FOUND\n", 15, 0);
            }

            pthread_mutex_unlock(&restaurant_mutex);
        }

        else {
            send(client_fd, "INVALID_COMMAND\n", 16, 0);
        }
            
        }
    

    close(client_fd);
    return NULL;
}
