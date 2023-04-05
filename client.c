#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_MESSAGE_LENGTH 1024
#define MAX_NAME_LENGTH 256

void* receive_messages(void* arg) {
    int client_socket = *(int*)arg;
    char message[MAX_MESSAGE_LENGTH];

    while (recv(client_socket, message, MAX_MESSAGE_LENGTH, 0) > 0) {
        printf("%s", message);
        memset(message, 0, MAX_MESSAGE_LENGTH);
    }

    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    inet_aton(SERVER_ADDRESS, &server_address.sin_addr);

    connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    char name[MAX_NAME_LENGTH];
    printf("Entrez votre nom : ");
    fgets(name, MAX_NAME_LENGTH, stdin);
    name[strcspn(name, "\n")] = 0;

    // Envoyer le nom au serveur
    send(client_socket, name, strlen(name), 0);

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &client_socket);

    char message[MAX_MESSAGE_LENGTH];
    while (fgets(message, MAX_MESSAGE_LENGTH, stdin) != NULL) {
        if (strcmp(message, "/quit\n") == 0) {
            break;
        }
        send(client_socket, message, strlen(message), 0);
        memset(message, 0, MAX_MESSAGE_LENGTH);
    }

    close(client_socket);
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    return 0;
}
