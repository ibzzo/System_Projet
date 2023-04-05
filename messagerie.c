#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 1024

// Structure de données pour stocker les informations sur chaque client
struct client_info {
    int socket; // socket du client
    int client_num; // numéro du client
    char messages[MAX_MESSAGES][MAX_MESSAGE_LEN]; // messages envoyés par le client
};

// Variables globales
struct client_info clients[MAX_CLIENTS]; // tableau de clients connectés
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex pour accéder à la matrice de messages

// Fonction pour ajouter un message à la matrice de messages pour un client donné
void add_message(int client_num, char* message) {
    pthread_mutex_lock(&mutex); // Verrouiller le mutex pour éviter les conflits
    int i;
    for (i = 0; i < MAX_MESSAGES; i++) {
        if (clients[client_num].messages[i][0] == '\0') {
            strcpy(clients[client_num].messages[i], message);
            break;
        }
    }
    pthread_mutex_unlock(&mutex); // Déverrouiller le mutex
}

// Fonction pour envoyer un message à tous les clients connectés
void send_message(char* message) {
    pthread_mutex_lock(&mutex); // Verrouiller le mutex pour éviter les conflits
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0) { // Si le client est connecté
            send(clients[i].socket, message, strlen(message), 0); // Envoyer le message
        }
    }
    pthread_mutex_unlock(&mutex); // Déverrouiller le mutex
}

// Fonction pour gérer la connexion d'un nouveau client
void* handle_client(void* arg) {
    int client_num = *(int*)arg; // Récupérer le numéro du client
    char buffer[MAX_MESSAGE_LEN];
    while (1) {
        memset(buffer, 0, MAX_MESSAGE_LEN); // Réinitialiser le buffer
        if (recv(clients[client_num].socket, buffer, MAX_MESSAGE_LEN, 0) <= 0) { // Si le client se déconnecte
            printf("Client %d disconnected.\n", client_num);
            close(clients[client_num].socket); // Fermer le socket du client
            clients[client_num].socket = 0; // Marquer le client comme déconnecté
            break;
        }
        add_message(client_num, buffer); // Ajouter le message à la matrice de messages du client
        printf("Received message from client %d: %s\n", client_num, buffer);
        send_message(buffer); // Envoyer le message à tous les clients connectés
    }
    pthread_exit(NULL); // Terminer le thread
}

int main(int argc, char** argv) {
    int server_socket, client_socket, client_num = 0;
    struct sockaddr_in server_address, client_address;
    pthread_t threads[MAX_CLIENTS];
    int opt = 1;

    return 0;
}

