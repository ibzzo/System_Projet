#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 1024
#define MAX_NAME_LENGTH 256

pthread_t threads[MAX_CLIENTS];
int client_count = 0;

// Matrice pour stocker les messages de chaque utilisateur
char messages[MAX_CLIENTS][MAX_MESSAGE_LENGTH];

// Mutex pour protéger l'accès à la matrice des messages
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Fonction pour ajouter un nouveau message à la matrice des messages
void add_message(int client_id, char* message) {
    pthread_mutex_lock(&mutex);
    strcpy(messages[client_id], message);
    pthread_mutex_unlock(&mutex);
}

// Fonction pour envoyer un message à tous les clients
void broadcast(int sender_id, char* message) {
    for (int i = 0; i < client_count; i++) {
        if (i != sender_id) {
            send(i, message, strlen(message), 0);
        }
    }
}

// Fonction pour traiter les messages reçus des clients
void* handle_client(void* arg) {
    int client_id = *(int*)arg;
    free(arg);

    // Envoie d'un message de bienvenue au client
    char welcome_message[MAX_MESSAGE_LENGTH];
    sprintf(welcome_message, "Bienvenue sur le chat ! Vous êtes l'utilisateur n°%d.\n", client_id);
    send(client_id, welcome_message, strlen(welcome_message), 0);

    // Boucle pour recevoir les messages du client
    char message[MAX_MESSAGE_LENGTH];
    while (recv(client_id, message, MAX_MESSAGE_LENGTH, 0) > 0) {
        // Ajout du message à la matrice des messages
        add_message(client_id, message);

        // Envoie du message à tous les clients
        char broadcast_message[MAX_MESSAGE_LENGTH + MAX_NAME_LENGTH];
        sprintf(broadcast_message, "Utilisateur n°%d : %s", client_id, message);
        broadcast(client_id, broadcast_message);

        memset(message, 0, MAX_MESSAGE_LENGTH);
    }

    // Suppression du client de la liste des clients connectés
    pthread_mutex_lock(&mutex);
    for (int i = client_id; i < client_count - 1; i++) {
        threads[i] = threads[i + 1];
        memset(messages[i], 0, MAX_MESSAGE_LENGTH);
        strcpy(messages[i], messages[i + 1]);
    }
    client_count--;
    pthread_mutex_unlock(&mutex);

    // Fermeture de la socket et fin du thread
    close(client_id);
    pthread_exit(NULL);
}
 int main(int argc, char const *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Création de la socket serveur
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Initialisation de la structure d'adresse serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // Liaison de la socket serveur à l'adresse serveur
bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

// Écoute de la socket serveur
listen(server_socket, MAX_CLIENTS);

printf("Le serveur est à l'écoute sur le port 8080...\n");

// Boucle d'attente de connexions clientes
while (1) {
    // Acceptation d'une connexion cliente
    printf("client");
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);

    // Vérification du nombre maximal de clients atteint
    if (client_count >= MAX_CLIENTS) {
        printf("Nombre maximal de clients atteint. Connexion refusée.\n");
        close(client_socket);
        continue;
    }

    // Ajout du client à la liste des clients connectés
    int* client_id = malloc(sizeof(int));
    *client_id = client_count;
    threads[client_count] = pthread_self();
    client_count++;

    // Création d'un thread pour traiter les messages du client
    pthread_create(&threads[client_count - 1], NULL, handle_client, (void*)client_id);
}

// Fermeture de la socket serveur
close(server_socket);

return 0;
}