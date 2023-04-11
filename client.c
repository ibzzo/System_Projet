#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 1024
#define MAX_NAME_LENGTH 256
int interrupted =0;
char message_buffer[MAX_MESSAGE_LENGTH * 10]; // tampon pour stocker les messages reçus pendant l'interruption

void* receive_messages(void* arg) 
    {
        int client_socket = *(int*)arg;
        char message[MAX_MESSAGE_LENGTH];

        while (read(client_socket, message, MAX_MESSAGE_LENGTH) > 0)
        {
            if(interrupted)
            {
                strcat(message_buffer, message);
                memset(message, 0, MAX_MESSAGE_LENGTH);
            }else
            {
                printf("%s",message);
            }

            memset(message, 0, MAX_MESSAGE_LENGTH);
        }

        pthread_exit(NULL);
    }


int main(int argc, char const *argv[]) 
{

    int client_socket = socket(AF_UNIX, SOCK_STREAM, 0);


    //innitialisation struct adress du serveur 
    struct sockaddr_un server_address = {0};
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path,"./MyStock");//chemin vers fichier socket

    if(connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address))<0)
    {
        printf("erreur \n");
        return -1;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &client_socket);

    char message[MAX_MESSAGE_LENGTH];
    char interrupt_message[] = "Interruption de l'affichage effectuée. Saisissez et Appuyez sur Entrée pour continuer.\n";
    while (fgets(message, MAX_MESSAGE_LENGTH, stdin) != NULL) {
        if (strcmp(message, "ctrl+c\n") == 0) 
        {
            interrupted = 1;
            printf("%s",interrupt_message);
            memset(message, 0, MAX_MESSAGE_LENGTH);
            fgets(message, MAX_MESSAGE_LENGTH, stdin);
            printf("%s",message_buffer);
            interrupted = 0;
        }
        if(write(client_socket, message, strlen(message)) < 0)
        {
            printf("Erreur lors de l'envoi du message");
            break;
        }
        if (strcmp(message, "bye!\n") == 0)
        {
            break;
        }
        memset(message, 0, MAX_MESSAGE_LENGTH);
    }

    close(client_socket);
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    return 0;
}