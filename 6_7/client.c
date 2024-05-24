#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

bool running = true;

void end_program(int signal) {
    if (signal == SIGINT) {
        printf("Ending program\n");
        running = false;
    }

}

void simulate_client(int id, const char* server_ip, int server_port) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Create error\n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address \n");
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection error\n");
        return;
    }

    int v = 1;
    send(sock, &v, 1, 0);

    int left = id;
    int right = (id + 1) % 5;
    srand(time(0) + id);

    while (running) {
        printf("Philosopher %d is thinking\n", id);
        sleep(rand() % 3 + 1);

        int action = 0;
        write(sock, &action, sizeof(action));
        write(sock, &left, sizeof(left));
        write(sock, &right, sizeof(right));

        char response[3];
        read(sock, response, 2);
        response[2] = '\0';

        if (strcmp(response, "OK") == 0) {
            printf("Philosopher %d is eating\n", id);
            sleep(rand() % 3 + 1);

            action = 1;
            write(sock, &action, sizeof(action));
            write(sock, &left, sizeof(left));
            write(sock, &right, sizeof(right));
        }
    }
    v = 2;
    send(sock, &v, 1, 0);
    close(sock);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Invalid input parameters\n");
        return 1;
    }

    signal(SIGINT, end_program);

    int id = atoi(argv[1]);
    const char* server_ip = argv[2];
    int server_port = atoi(argv[3]);

    simulate_client(id, server_ip, server_port);
}