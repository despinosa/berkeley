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

time_t send_time(int sock_desc);
void adjust(int sock_desc, time_t old_time);
struct sockaddr_in parse_addr(int argc, char const *argv[]);


int main(int argc, char const *argv[]) {
    int sock_desc;
    struct sockaddr_in host_addr;
    time_t old_time;

    host_addr = parse_addr(argc, argv);

    sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_desc < 0) {
        fprintf(stderr, "Error initializando socket (%d).\n", errno);
        exit(1);
    }

    if(connect(sock_desc, (struct sockaddr*) &host_addr, sizeof(host_addr)) < 0
            && errno != EINPROGRESS) {
        fprintf(stderr, "Error de conexión (%d).\n", errno);
        exit(1);
    }
    printf("Conectado a servidor.\n");
    old_time = send_time(sock_desc);
    adjust(sock_desc, old_time);
    return 0;
}



struct sockaddr_in parse_addr(int argc, char const *argv[]) {
    int host_port;
    const char *host_name;
    struct sockaddr_in host_addr;

    if(argc != 3) {
        printf("Uso: '%s <serv_name> <serv_port>'.\n", argv[0]);
        exit(1);
    }
    host_name = argv[1];
    host_port = atoi(argv[2]);

    memset((char *) &host_addr, 0, sizeof(host_addr));
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(host_port);
    memset(&(host_addr.sin_zero), 0, 8);
    host_addr.sin_addr.s_addr = inet_addr(host_name);
    return host_addr;
}


time_t send_time(int sock_desc) {
    time_t current_time;
    unsigned int converted;

    current_time = time(NULL);

    if(current_time == -1) {
        fprintf(stderr, "Error al obtener tiempo (%d).\n", errno);
        exit(1);
    }
    converted = htonl(current_time);

    printf("Enviando tiempo actual: %d.\n", (unsigned int) current_time);
    write(sock_desc, &converted, 32);
}


void adjust(int sock_desc, time_t old_time) {
    char buffer[25];
    float adjust;
    double new_time;

    read(sock_desc, buffer, 25);
    adjust = atof(buffer);

    new_time = old_time + adjust;
    printf("Ajuste de %+.5f.\n", adjust);
    if(new_time < 0) {
        fprintf(stderr, "Error de ajuste inválido.\n");
        exit(1);
    }
    printf("Nuevo tiempo %f.\n", new_time);
    old_time = (time_t) new_time;
    if(stime(&old_time) == -1) {
        fprintf(stderr, "Error al establecer la hora (%d).\n", errno);
        exit(1);
    }
}
