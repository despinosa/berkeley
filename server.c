#define _SVID_SOURCE /* glibc2 necesita ésto */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

struct client {
    int sock_desc;
    unsigned int time;
    float delta_t;
    clock_t recvd;
};


struct clients* first_serve(int server_desc, int clients_q);

int main(int argc, char const *argv[]) {
    int server_desc, port, sleep_timer, clients_q, opt;
    struct sockaddr_in server_addr;
    struct client* clients;

    if(argc != 4) {
        printf("Uso: '%s <port> <sleep_timer> <clients_q>'.\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);
    sleep_timer = atoi(argv[2]);
    clients_q = atoi(argv[3]);


    server_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(server_desc < 0) {
        fprintf(stderr, "Error initializando socket (%d).\n", errno);
        exit(1);
    }

    opt = 1;
    if((setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &opt,
            sizeof(int)) == -1) || (setsockopt(server_desc, SOL_SOCKET,
            SO_KEEPALIVE, &opt, sizeof(int)) == -1)) {
        fprintf(stderr, "Error al establecer opciones de socket (%d).", errno);
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_desc, (struct sockaddr*) &server_addr, sizeof(server_addr))
            < 0) {
        fprintf(stderr, "Error asociando socket (%d).\n", errno);
        exit(1);
    }

    clients = first_serve(server_desc, clients_q);
    do {
        sleep(sleep_timer * 1000);
        serve(clients, clients_q);
    } while(1);
    return 0;
}


struct client* first_serve(int server_desc, int clients_q) {
    char print_addr[INET_ADDRSTRLEN], buffer[25];
    unsigned int addr_len;
    float avg_time;
    struct client *clients;
    struct sockaddr_in cli_addr;


    clients = malloc(sizeof(struct client) * clients_q);
    listen(server_desc, clients_q);

    addr_len = sizeof(cli_addr);
    for (int i = 0; i < clients_q; ++i) {
        memset(&cli_addr, 0, addr_len);
        clients[i].sock_desc = accept(server_desc, (struct sockaddr*) &cli_addr,
                                      &addr_len);
        if(clients[i].sock_desc < 0 ) {
            fprintf(stderr, "Error aceptando conexión (%d).\n", errno);
            return;
        }
        inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, print_addr,
                  INET_ADDRSTRLEN);
        printf("Cliente conectado en %s.\n", print_addr);
        read(clients[i].sock_desc, &clients[i].time, sizeof(int));
        clients[i].recvd = clock();
        clients[i].time = ntohl(clients[i].time);
        printf("Hora de %s: %d.\n", print_addr, clients[i].time);
    }

    avg_time = clients[0].time;
    for (int i = 1; i < clients_q; ++i) {
        avg_time += clients[i].time - (clients[i].recvd - clients[0].recvd) /
                    CLOCKS_PER_SEC;
    }
    avg_time = avg_time / clients_q;

    for (int i = 0; i < clients_q; ++i) {
        clients[i].delta_t = avg_time - clients[i].time;
        sprintf(buffer, "%025.10f", clients[i].delta_t);
        write(clients[i].sock_desc, buffer, 25);
    }
    return clients;
}


void serve(struct client* clients, int clients_q) {
    char print_addr[INET_ADDRSTRLEN], buffer[25];
    float avg_time;
    struct sockaddr_in cli_addr;


    for (int i = 0; i < clients_q; ++i) {
        read(clients[i].sock_desc, &clients[i].time, sizeof(int));
        clients[i].recvd = clock();
        clients[i].time = ntohl(clients[i].time);
        printf("Hora de %s: %d.\n", print_addr, clients[i].time);
    }

    avg_time = clients[0].time;
    for (int i = 1; i < clients_q; ++i) {
        avg_time += clients[i].time - (clients[i].recvd - clients[0].recvd) /
                    CLOCKS_PER_SEC;
    }
    avg_time = avg_time / clients_q;

    for (int i = 0; i < clients_q; ++i) {
        clients[i].delta_t = avg_time - clients[i].time;
        sprintf(buffer, "%025.10f", clients[i].delta_t);
        write(clients[i].sock_desc, buffer, 25);
    }
}
