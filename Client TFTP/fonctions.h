#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>




#define ERREUR "Le nombre d'argument n'est pas valide, rappel : '%s host:port file'\n"
#define BIENVENUE "Connecté à %s sur le port %s pour le fichier %s\n"
#define PORT "69"
#define MAX_RRQ 128
#define MAX_WRQ 128
#define BUFFER_TAILLE 516
#define ACQ_TAILLE 4
#define ERREUR_SERVEUR "L'erreur vient du Serveur %s\n"

