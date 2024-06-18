#include "socket.h"
#include <mysql.h>

MYSQL* initialize_mysql() {
    MYSQL *con = mysql_init(NULL);
    if (con == NULL) {
        fprintf(stderr, "MySQL initialisation n'a pas marché : %s\n", mysql_error(con));
        return NULL;
    }

    if (mysql_real_connect(con, "localhost", "root", "vh.22082609", "MESSAGERIEPROJECT", 0, NULL, 0) == NULL) {
        fprintf(stderr, "Échec de connexion à la base de données : %s\n", mysql_error(con));
        mysql_close(con);
        return NULL;
    }

    // Configure session to use utf8mb4
    mysql_set_character_set(con, "utf8mb4");

    return con;
}



int main() {        

    int sock_serveur= creer_TCPI_IPv4_Socket();
    struct sockaddr_in* add_serveur = creer_IPv4_adresse ("" , 8888 );
    
   //en appelant la fonction bind() , on dit au S_Exploit que ce procecssus veut utiliser le port 8888 poue ecouter les nouvelles connections
    MYSQL *con = initialize_mysql();
    if (con == NULL) return EXIT_FAILURE;

    if (sock_serveur == -1 || bind(sock_serveur, (struct sockaddr*)add_serveur, sizeof(*add_serveur)) == -1 || listen(sock_serveur, 3) == -1) {
        perror("Server setup error");
        mysql_close(con);
        free(add_serveur);
        return EXIT_FAILURE;
    }
    printf("Le serveur est prêt à écouter sur le port %d\n", ntohs(add_serveur->sin_port));
  
    Commence_Accepter_Connections(sock_serveur);

 
    mysql_close(con);

    free(add_serveur);

    return 0;
}

