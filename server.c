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

        // Vérifier si le message est un message de commande
        if (message[0] == '/') {
            // Extraire la commande et les arguments
            char command[MAX_MESSAGE_LENGTH];
            char argument[MAX_MESSAGE_LENGTH];
            sscanf(message, "/%s %[^\n]", command, argument);

            // Gestion de la commande /quit
            if (strcmp(command, "quit") == 0) {
                // Envoyer un message de déconnexion au client
                char quit_message[MAX_MESSAGE_LENGTH];
                sprintf(quit_message, "Vous avez été déconnecté.\n");
                send(client_id, quit_message, strlen(quit_message), 0);

                // Supprimer le client de la liste des clients connectés
                pthread_mutex_lock(&mutex);
                for (int i = client_id; i < client_count - 1; i++) {
                    threads[i] = threads[i + 1];
                    memset(messages[i], 0, MAX_MESSAGE_LENGTH);
                    strcpy(messages[i], messages[i + 1]);
                }
                client_count--;
                pthread_mutex_unlock(&mutex);
            // Fermer la connexion
            close(client_id);
            return NULL;
        }
    }

    // Réinitialiser le message
    memset(message, 0, MAX_MESSAGE_LENGTH);
}

// Si le client ferme la connexion, supprimer le client de la liste des clients connectés
pthread_mutex_lock(&mutex);
for (int i = client_id; i < client_count - 1; i++) {
    threads[i] = threads[i + 1];
    memset(messages[i], 0, MAX_MESSAGE_LENGTH);
    strcpy(messages[i], messages[i + 1]);
}
client_count--;
pthread_mutex_unlock(&mutex);

// Fermer la connexion
close(client_id);
return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) {
    printf("Utilisation : %s port\n", argv[0]);
    return 1;
    }

    int port = atoi(argv[1]);

// Initialiser le socket
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

// Configuration du socket
struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_addr.s_addr = INADDR_ANY;
server_address.sin_port = htons(port);

// Lier le socket à l'adresse locale
bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address));

// Ecouter les connexions entrantes
listen(server_fd, MAX_CLIENTS);

printf("Serveur en écoute sur le port %d...\n", port);

// Boucle principale pour accepter les connexions entrantes
while (1) {
    // Accepter la connexion entrante
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_address_len);
    if(client_fd<0)
    {
        printf("Ereur d'acceptation");
    }

    // Vérifier si le nombre maximum de clients est atteint
    if (client_count >= MAX_CLIENTS) {
        printf("Nombre maximum de clients atteint. Connexion refusée.\n");
        close(client_fd);
        continue;
    }

    // Ajouter le nouveau client à la liste des clients connectés
    int* client_id_ptr = malloc(sizeof(int));
    *client_id_ptr = client_count;
    if(pthread_create(&threads[client_count], NULL, handle_client, (void*)client_id_ptr)<0)
    {
        printf("Erreur d'ajout");
    }
    client_count++;

    printf("Nouvelle connexion : %s:%d (utilisateur n°%d)\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), *client_id_ptr);
}

// Fermer le socket du serveur
close(server_fd);

return 0;

}