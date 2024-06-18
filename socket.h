
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <pthread.h>
#include <malloc.h>

#define MAX_CLIENTS 5
#define TAILLE_NOM 50

typedef struct {
    int sock;
    struct sockaddr_in address;
    socklen_t longueur_addr;
    char nom_cli[200];  
} structure_client;

typedef struct {
    structure_client *cli;
    MYSQL *conn; // un pointeur vers une structure de type MYSQL qui sera utilise par la bibliothèque client MySQL pour stocker les informations de connexion à une base de données MySQ
} client_thread_data;


structure_client tab_clients[MAX_CLIENTS];// un tableau des clients
int compteur_client = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;// declaration d'un mutex pour gerer l'acces au resources parteges par les multi_thread



// la fonction qui configure une adresse IPv4 a un Poert 
struct sockaddr_in* creer_IPv4_adresse (char *ip , int port )
{
    // allouer unespace memoire pour une struc sockaddr_in
    struct sockaddr_in *adresse = malloc(sizeof(struct sockaddr_in));
    if (!adresse) {
        perror(" Echec d'allocation ");
        return NULL;
    }    
    //config de la famille  l'adresse a IPv4
    adresse ->sin_family = AF_INET;

    // config du port , converti au format reseau 
    adresse ->sin_port = htons(port);
   
    if(strlen(ip)==0)
    {
       adresse ->sin_addr.s_addr = INADDR_ANY; //si auccune adresse ip n'est fournie, ecoute partout
    }
    else{
       // inet_pton Convertit l'adresse IP de texte à binaire
         if (inet_pton(AF_INET, ip, &adresse->sin_addr.s_addr) != 1) {
            perror("inet_pton a failli");
            free(adresse);
            return NULL;
    }

    return adresse ;
}
}

// Une fontion qui cree un Socket TCPI_IPv4 
int creer_TCPI_IPv4_Socket (){

    int sock_descript = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_descript == -1) {
           printf("Impossible de créer le socket");
           return 1;
        }
        puts("Socket créé");

     return sock_descript ;


}

bool verifier_et_reconnecter_mysql(MYSQL **con) {
    if (mysql_ping(*con)) {
        printf("La connexion a été perdue. Tentative de reconnexion...\n");
        mysql_close(*con); // Fermer la vieille connexion
        *con = mysql_init(NULL);
        if (*con == NULL || mysql_real_connect(*con, "localhost", "root", "vh.22082609", "MESSAGERIEPROJECT", 0, NULL, 0) == NULL) {
            fprintf(stderr, "Échec de la reconnexion : %s\n", mysql_error(*con));
            return false;
        }
        printf("Reconnexion réussie.\n");
    }
    return true;
}



// La fonction qui accepte les nounelles connections des clients 
structure_client* Accepter_nouv_connection(int descripteur_socket){
        
    if (descripteur_socket < 0) {
        fprintf(stderr, "Descripteur de socket invalide\n");
        return NULL;
    }
        
    // allouer un espace memoire pour une struc_client qui contient toutes les infos du cli y compris son adresse , son socket et la long de son adre
    structure_client *cli =malloc(sizeof(structure_client));
    if (!cli) {
        perror("Échec d'allocation mémoire pour un nouveau client");
        return NULL ;
    }
            
    // la fonction memset() assure que tous les champs de cli sont mis a zero pour eviter des err
    memset(cli, 0, sizeof(structure_client));

    cli->longueur_addr = sizeof(cli->address);

    // la fonction accept() return un descripteur d'un socket pour chaque client connecte , et etablie la conex de ce socket avec celui du serveur 
    cli->sock = accept(descripteur_socket, (struct sockaddr *)&cli->address, &cli->longueur_addr);
            
    if (cli->sock < 0) {
        perror("Échec de l'acceptation!!");
        free(cli);
        return NULL;
    }
    

    return cli ;
}


//La fonction Responsable de broadcaster les messages
void Evoyer_Groupe(structure_client *cli, const char *message)
{
   
    pthread_mutex_lock(&clients_mutex);//verouiller le mutex(si le mutex est deja verouillé par un autre thread , le thread demandeur sera mis en attente jusqu'a que le mutex soit verouille)
   
    for (int i = 0; i < compteur_client; i++)
    {
        if (tab_clients[i].sock != cli->sock)
        {
            if (send(tab_clients[i].sock, message, strlen(message), 0) < 0) {
                perror("Échec de l'envoi du message");
            }
        
        }
    }

    pthread_mutex_unlock(&clients_mutex);//déverrouiller le mutex 
}

// la fonction responsable de sauvegarder le messages dans la base des donnees sql
void sauvegarde_message(MYSQL *con, const char* username, const char* message) {

 
    char requete[1024];// un tableau pour y stocker les requetes sql
    
    sprintf(requete, "INSERT INTO messages (username, message) VALUES('%s', '%s')", username, message);
    
    //mysql_query est utilisée pour envoyer des commandes SQL au serveur MySQL en utilisant le pointeur conn pour exécuter la requête sur la connexion établie
    if (mysql_query(con, requete)) {
        fprintf(stderr, "%s\n", mysql_error(con));
    }
}


// La fonction qui recoit les messages venant ds clients et envoyer aux destinateurs
void *Recevoir_Affiecher_Messages_des_clients(void *arg) {
   
    client_thread_data *data = (client_thread_data *)arg;
    structure_client *cli = data->cli;
   
    MYSQL *con = data->conn;
    
    char message[2002];
    int nbr_byts_rçus;

    while ((nbr_byts_rçus = recv(cli->sock, message, sizeof(message), 0)) > 0) {
       
        message[nbr_byts_rçus] = '\0';
        printf("message recu de %s : %s\n", cli->nom_cli, message);
        if (!verifier_et_reconnecter_mysql(&data->conn)) {
           continue; // ou gérer comme nécessaire
         }
        sauvegarde_message(con, cli->nom_cli, message);
        char *contenu = strchr(message, ':');// ici on chercher la 1ere occurence du ':' dans le message du client,la fonction strchr() va returner un pointeur vers ':'  
       
        // ici on traite le cas ou on a trouve une occurence de ':'
        if (contenu) {
           
           *contenu = '\0';
            char nom_destinataire[TAILLE_NOM];
            strncpy(nom_destinataire, message, sizeof(nom_destinataire));
            nom_destinataire[sizeof(nom_destinataire) - 1] = '\0';
            contenu++;

            printf("destinataire extrait : %s\n", nom_destinataire);

            if (strcmp(nom_destinataire, "All") == 0) {
                Evoyer_Groupe(cli, contenu);
            }
            
            else {
                
                bool trouve = false;
                pthread_mutex_lock(&clients_mutex);
                
                for (int i = 0; i < compteur_client; i++) {
                  if(strcmp(tab_clients[i].nom_cli, nom_destinataire) == 0){
                      if(send(tab_clients[i].sock, contenu, strlen(contenu), 0) >= 0) {
                        printf("Message envoyé à %s : %s\n", tab_clients[i].nom_cli, contenu);
                        trouve = true;
                        break;
                      }
                    }
                }
                pthread_mutex_unlock(&clients_mutex);
                if (!trouve) {
                    printf("Destinataire '%s' non trouvé.\n", nom_destinataire);
                }
            }
        } 
        
        // le cas contraire
        else {
            printf("Format de message invalide reçu de %s : %s\n", cli->nom_cli, message);
        }
    }

    if (nbr_byts_rçus == 0) {
        printf("Client %s déconnecté.\n", cli->nom_cli);
    } else if (nbr_byts_rçus == -1) {
        perror("Erreur de réception");
    }

    close(cli->sock);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < compteur_client; i++) {
        if (tab_clients[i].sock == cli->sock) {
            tab_clients[i] = tab_clients[compteur_client - 1];
            compteur_client--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    mysql_close(con);
    close(cli->sock);
    free(data);
    return NULL;

}



//cette fonction appele la fonction Recevoir_Affiecher_Messages_des_clients() avec differents threads
void traiter_nouveau_client_avec_diff_threads(structure_client *cli) {
    
    client_thread_data *data = malloc(sizeof(client_thread_data));
    if (data == NULL) {
        perror("Erreur de l'allocation de la memoire pour thread data");
        return;
    }

    data->cli = cli;
    data->conn = mysql_init(NULL);//allouer une structure mysql 
    if (!data->conn || !mysql_real_connect(data->conn, "localhost", "root", "vh.22082609", "MESSAGERIEPROJECT", 0, NULL, 0)) {
        fprintf(stderr, "Échec de la connexion à la base de données : %s\n", mysql_error(data->conn));
        if (data->conn) mysql_close(data->conn);
        free(data);
        close(cli->sock);
        free(cli);
        return;
    }

    pthread_t id;
    if (pthread_create(&id, NULL, Recevoir_Affiecher_Messages_des_clients, data) != 0) {
        perror("Échec de création du thread");
        mysql_close(data->conn);
        free(data);  
        close(cli->sock);
        free(cli);
        return;      
    }
    
     else {
        pthread_detach(id); // la fonction pthread_detach() permettre à un thread de gérer ses propres ressources et de terminer de manière autonom
    }  
}

bool verifier_utilisateur(MYSQL *con, const char *username, const char *password) {
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[1], bind_out[1];
    char db_password[255] = {0};
    bool authenticated = false;

    const char *query = "SELECT password FROM utilisateur WHERE username = ?";
    stmt = mysql_stmt_init(con);
    if (!stmt) {
        fprintf(stderr, "mysql_stmt_init(), out of memory\n");
        return false;
    }
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "mysql_stmt_prepare(), SELECT failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char *)username;
    bind[0].buffer_length = strlen(username);

    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr, "mysql_stmt_bind_param() failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    memset(bind_out, 0, sizeof(bind_out));
    bind_out[0].buffer_type = MYSQL_TYPE_STRING;
    bind_out[0].buffer = db_password;
    bind_out[0].buffer_length = 255;

    if (mysql_stmt_bind_result(stmt, bind_out)) {
        fprintf(stderr, "mysql_stmt_bind_result() failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) || mysql_stmt_store_result(stmt)) {
        fprintf(stderr, "Failed to execute or store result: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_num_rows(stmt) > 0) {
        while (mysql_stmt_fetch(stmt) == 0) {
            authenticated = (strcmp(password, db_password) == 0);
        }
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    return authenticated;
}




bool ajouter_utilisateur(MYSQL *con, const char *username, const char *password) {
    MYSQL_STMT *stmt;
    char *requete = "INSERT INTO utilisateur (username, password) VALUES (?, ?)";
    stmt = mysql_stmt_init(con);
    if (stmt == NULL) {
        fprintf(stderr, "Échec de l'initialisation de la déclaration: %s\n", mysql_error(con));
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, requete, strlen(requete)) != 0) {
        fprintf(stderr, "Échec de la préparation de la déclaration: %s\n", mysql_error(con));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (void *)username;
    params[0].buffer_length = strlen(username);
    
    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (void *)password;
    params[1].buffer_length = strlen(password);

    if (mysql_stmt_bind_param(stmt, params) != 0) {
        fprintf(stderr, "Échec de la liaison des paramètres: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        fprintf(stderr, "Erreur lors de l'exécution de la déclaration: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}



bool process_login(int sock, MYSQL *con) {
    char username[20], password[10], buffer[1024];
    int len;
    int received;

    // Demander le nom d'utilisateur
    len = sprintf(buffer, "Entrer votre username: ");
    send(sock, buffer, len, 0);
    received = recv(sock, username, sizeof(username) - 1, 0); // Laisser de la place pour le caractère nul final
    if (received > 0) {
        username[received] = '\0'; // Assurez-vous que la chaîne est terminée
        username[strcspn(username, "\r\n")] = 0; // Retirer les retours chariot et les nouvelles lignes
    } else {
        // Gérer l'erreur ou la déconnexion
        send(sock, "Erreur de réception du nom d'utilisateur.\n", 40, 0);
        return false;
    }

    // Demander le mot de passe
    len = sprintf(buffer, "Entrer votre password: ");
    send(sock, buffer, len, 0);
    received = recv(sock, password, sizeof(password) - 1, 0); // Laisser de la place pour le caractère nul final
    if (received > 0) {
        password[received] = '\0'; // Assurez-vous que la chaîne est terminée
        password[strcspn(password, "\r\n")] = 0; // Retirer les retours chariot et les nouvelles lignes
    } else {
        // Gérer l'erreur ou la déconnexion
        send(sock, "Erreur de réception du mot de passe.\n", 36, 0);
        return false;
    }

    // Vérifier les informations d'identification
    if (verifier_utilisateur(con, username, password)) {
        send(sock, "Authentification réussie.\n", 26, 0);
        return true; // Authentification réussie
    } else {
        send(sock, "Login impossible.\n", 19, 0);
        return false; // Authentification échouée
    }
}


bool process_signup(int sock, MYSQL *con) {
    char username[100], password[100], buffer[1024];
    int len;

    // Demander un nouveau nom d'utilisateur
    len = sprintf(buffer, "choisi un username: ");
    send(sock, buffer, len, 0);
    recv(sock, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0; // supprimer le \n

    // Demander un nouveau mot de passe
    len = sprintf(buffer, "Choisi un password: ");
    send(sock, buffer, len, 0);
    recv(sock, password, sizeof(password), 0);
    password[strcspn(password, "\n")] = 0; //supprimer le \n

    // Ajouter l'utilisateur à la base de données
    if (ajouter_utilisateur(con, username, password)) {
        return true; // Inscription réussie
    } 
    
    else {
        send(sock, "Signup impossible .\n", 20, 0);
        return false; // Échec de l'inscription
    }
}



//cette fonction est responsable d'accepter les nouvelles connections des clients en appelant les fonctions Accepter_nouv_connection() et traiter_nouveau_client_avec_diff_threads
void Commence_Accepter_Connections(int serverSocketFD) {

    while(1)
    {
        structure_client* cli  = Accepter_nouv_connection(serverSocketFD);
        
        if (cli == NULL)
        {
           continue; // Échec de la connexion, reprendre l'écoute
        }
        
        client_thread_data* data =(client_thread_data*)malloc(sizeof(client_thread_data));
        if (data == NULL) {
          perror("Erreur de l'allocation de la memoire pour thread data");
          close(cli->sock);
          free(cli);
          return;
        }       
        
        
        data->cli=cli ;
        data->conn = mysql_init(NULL);
        MYSQL *con = data->conn;
                
        /*if (!data->conn) {
           perror("Allocation impssible pour  client_thread_data");
           free(data);
           close(cli->sock);
           free(cli);
          continue;
        }       */
       
  
   /* if (!mysql_real_connect(data->conn, "localhost", "root", "password", "MESSAGERIEPROJECT", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect to database: %s\n", mysql_error(data->conn));
        mysql_close(data->conn);
        close(cli->sock);
        free(cli);
        free(data);
        continue;
    }*/
        

        // Interaction initiale pour la connexion ou l'inscription
        char buffer[1024];
        int bytes_sent = send(cli->sock, "Connect (1) or Sign-Up (2): ", 28, 0);
        if (bytes_sent < 0) {
            perror("Send failed");
            mysql_close(data->conn);
            close(cli->sock);
            free(cli);
            free(data);
            continue;
        }
        
        char response[2];
        int bytes_received = recv(cli->sock, response, 2, 0);
        if (bytes_received < 0) {
            perror("Recv failed");
            mysql_close(data->conn);
            close(cli->sock);
            free(cli);
            free(data);
            continue;
        }

        if (response[0] == '1') {
            if (!verifier_et_reconnecter_mysql(&con) || !process_login(cli->sock, con)) {
                close(cli->sock);
                mysql_close(data->conn);
                free(cli);
                free(data);
                continue;
            }
        } else if (!verifier_et_reconnecter_mysql(&con) || response[0] == '2') {
            if (!process_signup(cli->sock, con)) {
                close(cli->sock);
                mysql_close(data->conn);
                free(cli);
                free(data);
                continue;
            }
        } else {
            close(cli->sock);
            mysql_close(data->conn);
            free(cli);
            free(data);
            continue;
        }

        //S'assurer que le nombre de client accepte ne soit pas depasse
        if (compteur_client < MAX_CLIENTS){
          
             printf("Veuillez entrer votre nom:");
             scanf("%s", cli->nom_cli);
             tab_clients[compteur_client++] = *cli; // ajouter le nouveau client connecte dans le tableau des client 
           
             traiter_nouveau_client_avec_diff_threads(cli);
        }
        
        else {
            printf("le Max des clients est atteints");
            close(cli->sock);
            mysql_close(data->conn);
            free(cli);
            free(data);            
        }
    
    }
}


// cette fonction lit les messages de la console  et les envoyer au serveur  
void* lire_messages_de_la_console_envoiyer (void* arg){
    
    int descri_socket=*((int*)arg);
    char *line = NULL;
    size_t lineSize= 0;
    printf("tape ton message (tape sortir si tu veux deconnecter)...\n");

    char buffer[1024];

    
    while (1) {
      
        // getline() va returner le nbr de caracteres tapes par le client dans la console
        ssize_t  nbr_caracteres = getline(&line,&lineSize,stdin);
        line[nbr_caracteres-1]='\0';

        sprintf(buffer,"%s",line);

        if(nbr_caracteres>0)
        {
            if (strcmp(line, "sortir") == 0) {
                printf("Vous avez demandé la déconnexion.\n");
                close(descri_socket); // Fermer le socket
                free(line); // Libérer la mémoire allouée pour la ligne
                exit(0);
          }
          
          ssize_t amountWasSent = send(descri_socket, buffer, strlen(buffer), 0);
        }
    }
 
    free(line);
}


//cette fonction recoit les reponses du servuer au clients 

void* Recevoir_Reponses_serv(void* arg)
{
    int descri_socket=*((int*)arg);
    char repo_serveur[8000];

    ssize_t longueur;

    while ((longueur = recv(descri_socket, repo_serveur, sizeof(repo_serveur), 0)) > 0) {
        repo_serveur[longueur] = '\0';  
        printf("Réponse du serveur : %s\n", repo_serveur);
    }

    if (longueur == 0) {
        printf("Le serveur a fermé la connexion.\n");
    } 
    else if (longueur < 0) {
        perror("Erreur de réception");
    }
    
   
    close(descri_socket);
    return NULL;
}



