#include "socket.h"

int main() {
    int socket_client = creer_TCPI_IPv4_Socket();
    struct sockaddr_in* serveur = creer_IPv4_adresse("127.0.0.1", 8888);
    pthread_t id ,id_1 ;
    
    // Connexion au serveur
    int result_conn = connect(socket_client, (struct sockaddr *)serveur, sizeof(*serveur));
    printf("%d\n", result_conn);
    if (result_conn < 0) {
        perror("echec de connection");
        close(socket_client);
        return 1;
    } else {
        printf("connexion au serveur reussie.\n");
    }

    
    if (pthread_create(&id, NULL, lire_messages_de_la_console_envoiyer, (void*)&socket_client) != 0) {
        perror("Erreur de creation du thread");
    }

    
    if (pthread_create(&id_1, NULL, Recevoir_Reponses_serv, (void*)&socket_client) != 0) {
        perror("Échec de la création du thread for receiving messages");
    }

    // Attendez que les threads se terminent
    pthread_join(id, NULL);
    pthread_join(id_1, NULL);

    close(socket_client);
    return 0;
}
