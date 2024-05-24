#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

pthread_mutex_t forks[5];
pthread_cond_t cond[5];
pthread_mutex_t monitor = PTHREAD_MUTEX_INITIALIZER;
int monitor_socket;

void state_scan() {
    pthread_mutex_lock(&monitor);
    for (int i = 0; i < 5; ++i) {
        int fork_state = (pthread_mutex_trylock(&forks[i]) == 0);
        pthread_mutex_unlock(&forks[i]);
        send(monitor_socket, &fork_state, sizeof(fork_state), 0);
    }
    pthread_mutex_unlock(&monitor);
}

void client_handle(int socket) {
    int action, left, right;

    while (1) {
        read(socket, &action, sizeof(int));
        read(socket, &left, sizeof(int));
        read(socket, &right, sizeof(int));

        if (action == 0) {
            pthread_mutex_lock(&forks[left]);
            pthread_mutex_lock(&forks[right]);
            write(socket, "OK", 2);

            printf("fork %d is locked\n", left);
            printf("fork %d is locked\n", right);
            state_scan();
        }
        else if (action == 1) {
            pthread_mutex_unlock(&forks[right]);
            pthread_mutex_unlock(&forks[left]);
            pthread_cond_signal(&cond[left]);
            pthread_cond_signal(&cond[right]);

            printf("fork %d is unlocked\n", left);
            printf("fork %d is unlocked\n", right);
            state_scan();
        }
        else if (action == 2) {
            break;
        }
    }
    close(socket);
}

void monitor_handle(int socket) {
    pthread_mutex_lock(&monitor);
    monitor_socket = socket;
    pthread_mutex_unlock(&monitor);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Invalid input parameters\n");
        return 1;
    }

    const char* server_ip = argv[1];
    const char* server_port = argv[2];

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addr_size = sizeof(address);
    int opt = 1;

    for (int i = 0; i < 5; ++i) {
        pthread_mutex_init(&forks[i], NULL);
        pthread_cond_init(&cond[i], NULL);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Sockopt error");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(atoi(server_port));

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addr_size)) < 0) {
            perror("accept error");
            exit(EXIT_FAILURE);
        }

        int client_type;
        read(client_socket, &client_type, sizeof(client_type));

        if (client_type == 1) {
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, (void*)client_handle, (void*)(intptr_t)client_socket);
        }
        else if (client_type == 2) {
            monitor_handle(client_socket);
        }


    }

    for (int i = 0; i < 5; i++) {
        pthread_mutex_destroy(&forks[i]);
        pthread_cond_destroy(&cond[i]);
    }

    close(server_fd);
}