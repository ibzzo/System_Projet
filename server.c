#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>


#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 1024
#define MAX_NAME_LENGTH 256

pthread_t threads[MAX_CLIENTS];
int clients_id[MAX_CLIENTS];
int client_count = 0;

// Matrice pour stocker les messages de chaque utilisateur
char messages[MAX_CLIENTS][MAX_MESSAGE_LENGTH];

// Mutex pour protéger l'accès à la matrice des messages
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Fonction pour ajouter un nouveau message à la matrice des messages
void ajout_message(int client_id, char* message) {
    pthread_mutex_lock(&mutex);
    strcpy(messages[client_id], message);
    pthread_mutex_unlock(&mutex);
}

// Fonction pour envoyer un message à tous les clients
void PropageMessage(int sender_id, char* message) {
    for (int i = 0; i < client_count; i++) {
        if (clients_id[i] != sender_id) {
            write(clients_id[i], message, strlen(message));
        }
    }
}

// Fonction pour traiter les messages reçus des clients
void* gereClient(void* arg) 
{
    int client_id = *(int*)arg;

    // Envoie d'un message de bienvenue au client
    char welcome_message[MAX_MESSAGE_LENGTH];
    sprintf(welcome_message, "Bienvenue sur le chat ! Vous êtes l'utilisateur n°%d.\n", client_id-3);
    if (send(client_id, welcome_message, strlen(welcome_message), 0)<0)
    {
        printf("Impossible d'envoyer le message de bienvenue.\n");
    }

    // Boucle pour recevoir les messages du client
    char message[MAX_MESSAGE_LENGTH];
    while (read(client_id, message, MAX_MESSAGE_LENGTH) > 0) 
    {
        // Ajout du message à la matrice des messages
        ajout_message(client_id, message);

        printf("Un mesage :%s\n",message);

        // Envoie du message à tous les clients
        char PropageMessage_message[MAX_MESSAGE_LENGTH + MAX_NAME_LENGTH];
        sprintf(PropageMessage_message, "user n°%d : %s", client_id-3, message);
        PropageMessage(client_id, PropageMessage_message);
        // Reinitialiser le message
        memset(message, 0, MAX_MESSAGE_LENGTH);
    }

    // Si le client ferme la connexion, supprimer le client de la liste des clients connectés
    pthread_mutex_lock(&mutex);
    for (int i = client_id-4; i < client_count - 1; i++) 
    {
        clients_id[i] = clients_id[i + 1];
        memset(messages[i], 0, MAX_MESSAGE_LENGTH);
        strcpy(messages[i], messages[i + 1]);
    }
    client_count--;
    pthread_mutex_unlock(&mutex);

    // Fermer la connexion
    close(client_id);
    pthread_exit(NULL);
}

int main(int argc, char** argv) 
{
    int secoute, sservice;
    struct sockaddr_un saddr = {0};
    saddr.sun_family=AF_UNIX;//saddr family unix
    strcpy(saddr.sun_path,"./MyStock");
    socklen_t addrlen = sizeof(saddr);
     
    // Création de la socket serveur
    if ((secoute = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    unlink("./MyStock");

    if (bind(secoute, (struct sockaddr *)&saddr, addrlen) == -1) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Écoute sur la socket serveur
    if (listen(secoute, 6) == -1) {   //6 : longeur maxmal  de la file d'atente
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Le serveur est en attente de connexions entrantes...\n");

    // Boucle principale de gestion des connexions
    while (1) {
        // Acceptation d'une connexion entrante
        struct sockaddr_un client_addr = {0};
        socklen_t client_addr_len = sizeof(client_addr);
        sservice = accept(secoute, (struct sockaddr*)&client_addr, &client_addr_len);
        if (sservice == -1) 
        {
            perror("Erreur lors de l'acceptation de la connexion entrante");
            return 1;
        }

        printf("Connexion acceptée\n");

        // Allocation d'un identifiant unique pour le client
        clients_id[client_count]=sservice;

        // Création d'un nouveau thread pour le client
        if (pthread_create(&threads[client_count], NULL, gereClient, &sservice) != 0) {
            perror("Erreur lors de la création du thread client");
            return 1;
        }

        // Incrémentation du compteur de clients
        client_count++;
    }

    // Fermeture de la socket serveur
    close(secoute);

    return 0;
}

