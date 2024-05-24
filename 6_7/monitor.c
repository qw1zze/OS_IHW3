#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

void monitor(char* server_ip, int server_port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket error\n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection error\n");
        return;
    }

    int client_type = 2;
    send(sock, &client_type, 1, 0);

    int fork_states[5];

    while (1) {
        for (int i = 0; i < 5; i++) {
            if (recv(sock, &fork_states[i], sizeof(fork_states[i]), 0) <= 0) {
                close(sock);
                return;
            }
        }

        printf("Forks: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", fork_states[i]);
        }
        printf("\n");

        sleep(1);
    }
    close(sock);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Invalid input parameters\n");
        return 1;
    }

    char* server_ip = argv[1];
    int server_port = atoi(argv[2]);

    monitor(server_ip, server_port);
}