#include "tftp.h"

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        printf("%s", USAGE);
        return 0;
    }
    // Adresse du serveur
    char* addr = (char*) argv[1];

    // Port (69 par defaut)
    char* port = extractPort((char*) argv[1]);

    // Commande de l'utilisatuer ("get" ou "put")
    char* cmd = (char*) argv[2];

    // Nom du fichier
    char* filename = (char*) argv[3];

    if (strcmp(cmd, "get") == 0) {
        getFile(filename, addr, port);
    } else if (strcmp(cmd, "put") == 0) {
        putFile(filename, addr, port);
    } else {
        printf("%s", USAGE);
    }
    return 0;
}

/**
 * Permet dee découper un chaine du type "xxx.xxx.xxx.xxx:xxx" en adresse
 * et port. La chaine donnée en entrée est troquée et la fonction renvoit
 * le port. Si ne port n'est pas spécifié, le port par défaut est utilisé.
 * 
 * @param addr Adresse avec port sépraré par deux points.
 * @return char* Le port
 */
char* extractPort(char* addr) {
    char* ptr = strchr(addr, ':');
    if (ptr == NULL) {
        return (char*) DEFAULT_PORT;
    } else {
        *ptr = '\0';
        ptr += 1;
        return ptr;
    }
}

/**
 * Cette fonction télécharge le fichier `filename` sur `addr`:`port`, s'il
 * existe.
 * 
 * @param filename Le nom du fichier.
 * @param addr L'adresse du serveur
 * @param port Le port à utiliser
 */
void getFile(char* filename, char* addr, char* port) {
    struct addrinfo* servinfo;
    int localFd, socketFd;

    // Demande du fichier
    sendReadRequest(filename, addr, port, &servinfo, &localFd, &socketFd);

    // Téléchargement des paquets 
    int ret = receiveAndAcknowledge(localFd, socketFd);

    close(socketFd);
    close(localFd);

    if (ret == 0)
        printf("Le fichier %s a été téléchargé avec succès.\n", filename);
    else {
        printf("Le fichier %s n'a pas pu être téléchargé.\n", filename);
        exit(EXIT_FAILURE);
    }
}

/**
 * Initie une connection avec le serveur spécifié afin de télécharger un fichier.
 * 
 * @param filename Nom du fichier à télécharger
 * @param addr Adresse du serveur
 * @param port Port à utiliser
 * @param servinfo Informations concenant le serveur
 * @param localFd Descripteur du fichier local (destination des données)
 * @param socketFd Descripteur du socket (source des données)
 */
void sendReadRequest(char* filename, char* addr, char* port, struct addrinfo** servinfo,
                     int* localFd, int* socketFd) {
    printf("Demande de %s sur %s:%s...\n", filename, addr, port);

    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(addr, port, &hints, servinfo)) {
        printf("Erreur : serveur inconnu.\n");
        exit(EXIT_FAILURE);
    }

    *localFd = open(filename, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRUSR|S_IRGRP|S_IROTH);
    *socketFd = socket((*servinfo)->ai_family, (*servinfo)->ai_socktype, (*servinfo)->ai_protocol);

    char* requestPacket;
    int requestPacketLength;
    formulateReadRequest(filename, &requestPacket, &requestPacketLength);

    int sent = sendto(*socketFd, requestPacket, requestPacketLength, 0, (*servinfo)->ai_addr, (*servinfo)->ai_addrlen);
	if (sent == -1){
		printf("Erreur : la demande n'a pu être effectuée.\n");
		exit(EXIT_FAILURE);
	}

    free(requestPacket);
}

/**
 * Forumle une demande de lecture. Paquet "RRQ".
 * 
 * @param filename Le fichier à télécharger.
 * @param requestPacket L'adresse du paquet qui sera généré.
 * @param requestPacketLength Sa longueur.
 */
void formulateReadRequest(char* filename, char** requestPacket, int* requestPacketLength) {
    *requestPacketLength = 2 + strlen(filename) + 1 + strlen("octet") + 1;
    *requestPacket = (char*) malloc(*requestPacketLength * sizeof(char));

    // Opcode (RRQ)
    (*requestPacket)[0] = 0;
    (*requestPacket)[1] = 1;

    // Filename
    strcpy(*requestPacket + 2, filename);

    // Mode
    strcpy(*requestPacket + 2 + strlen(filename) + 1, "octet");
}

/**
 * Réceptionne les paquets de données un par un en les aquitant pour que le serveur 
 * envoit les suivants.
 * 
 * @param localFd Fichier à envoyer.
 * @param socketFd Socket vers le serveur.
 * @return int Signal.
 */
int receiveAndAcknowledge(int localFd, int socketFd) {
    struct sockaddr address;
    socklen_t addressSize = sizeof(address);
    unsigned char buffer[BUFFER_SIZE];
    int blockNumber = 1;
    int count = BUFFER_SIZE;

    while (count == BUFFER_SIZE) {
        count = recvfrom(socketFd, buffer, BUFFER_SIZE, 0, &address, &addressSize);
        if (count == -1) {
            printf("Erreur : mauvaise reception du paquet %d.\n", blockNumber);
            return -1;
        }

        printf("Paquet %d reçu. (%d octets)\n", blockNumber, count);

        if (buffer[0]==0 && buffer[1]==5) {
            printf("Une erreur s'est produite.\n");
            return -1;
        }

        char acq[ACQ_SIZE];

        if (buffer[0] == 0 && buffer[1] == 3) {
            acq[0] = 0;
            acq[1] = 4;
            acq[2] = buffer[2];
            acq[3] = buffer[3];
        } else {
            printf("Le paquet reçu n'est pas un paquet de données.\n");
            return -1;
        }
        
        int sendacq = sendto(socketFd, acq, ACQ_SIZE, 0, &address, addressSize);
        if (sendacq == -1) {
            printf("Erreur lors de l'acquittement.\n");
            return -1;
        }

        if ((buffer[2]<<8) + buffer[3] == blockNumber) {
            if (write(localFd, buffer+4, count-4) == -1) {
                printf("Erreur : Les données n'ont pas pu être sauvegardées.\n");
            }
            blockNumber++;
        } else {
            printf("Erreur : Le numero de paquet ne correspond pas au paquet attendu.\n");
        }
    }
    return 0;
}

/**
 * Téléverse un fichier local vers un serveur.
 * 
 * @param filename Le fichier.
 * @param addr Le serveur.
 * @param port Le port.
 */
void putFile(char* filename, char* addr, char* port) {
    struct addrinfo* servinfo;
    int localFd, socketFd;

    // Demande de l'envoi du fichier
    sendWriteRequest(filename, addr, port, &servinfo, &localFd, &socketFd);

    // Téléchargement des paquets 
    int ret = sendAndWaitAcknowledgement(localFd, socketFd);

    close(socketFd);
    close(localFd);

    if (ret == 0)
        printf("Le fichier %s a été téléversé avec succès.\n", filename);
    else {
        printf("Le fichier %s n'a pas pu être téléversé.\n", filename);
        exit(EXIT_FAILURE);
    }
}

/**
 * Ouvre une connection au serveur pour l'envoi d'un fichier.
 * 
 * @param filename Le fichier.
 * @param addr L'adresse du serveur.
 * @param port Le port à utiliser.
 * @param servinfo La connection.
 * @param localFd Le descripteur du fichier local;
 * @param socketFd Le descripteur du socket.
 */
void sendWriteRequest(char* filename, char* addr, char* port, struct addrinfo** servinfo,
                     int* localFd, int* socketFd) {
    printf("Demande d'envoi de %s sur %s:%s...\n", filename, addr, port);

    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;


    if (getaddrinfo(addr, port, &hints, servinfo)) {
        printf("Erreur : serveur inconnu.\n");
        exit(EXIT_FAILURE);
    }

    *localFd = open(filename, O_RDONLY, 0);
    *socketFd = socket((*servinfo)->ai_family, (*servinfo)->ai_socktype, (*servinfo)->ai_protocol);

    char* requestPacket;
    int requestPacketLength;
    formulateWriteRequest(filename, &requestPacket, &requestPacketLength);

    int sent = sendto(*socketFd, requestPacket, requestPacketLength, 0, (*servinfo)->ai_addr, (*servinfo)->ai_addrlen);
	if (sent == -1){
		printf("Erreur : la demande n'a pu être effectuée.\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Forumle une demande d'écrture. Paquet "WRQ".
 * 
 * @param filename Le fichier à envoyer.
 * @param requestPacket L'adresse du paquet qui sera généré.
 * @param requestPacketLength Sa longueur.
 */
void formulateWriteRequest(char* filename, char** requestPacket, int* requestPacketLength) {
    *requestPacketLength = 2 + strlen(filename) + 1 + strlen("octet") + 1;
    *requestPacket = (char*) malloc(*requestPacketLength * sizeof(char));

    // Opcode (WRQ)
    (*requestPacket)[0] = 0;
    (*requestPacket)[1] = 2;

    // Filename
    strcpy(*requestPacket + 2, filename);

    // Mode
    strcpy(*requestPacket + 2 + strlen(filename) + 1, "octet");
}

/**
 * Envoit les paquets de données un par un en vérifiant que le serveur les 
 * a bien reçus.
 * 
 * @param localFd Fichier à envoyer.
 * @param socketFd Socket vers le serveur.
 * @return int Signal.
 */
int sendAndWaitAcknowledgement(int localFd, int socketFd) {
    struct sockaddr address;
    socklen_t addressSize = sizeof(address);
    char buffer[BUFFER_SIZE];
    int blockNumber = 1;
    int count = BUFFER_SIZE;

    char aqc[ACQ_SIZE];

    recvfrom(socketFd, aqc, ACQ_SIZE, 0, &address, &addressSize);
    if (aqc[0] == 0 && aqc[1] == 4 && aqc[2] == 0 && aqc[3] == 0) {
        printf("Demande acceptée.\n");
    }

    while (count == BUFFER_SIZE) {
        buffer[0] = 0;
        buffer[1] = 3;
        buffer[2] = blockNumber >> 8;
        buffer[3] = blockNumber & 0xff;
        
        count = read(localFd, buffer+4, BUFFER_SIZE-4);
        if (count == -1) {
            printf("Erreur : mauvaise lecture du fichier.\n");
            return -1;
        }
        if (count == BUFFER_SIZE-4)
            count += 4;

        int sent = sendto(socketFd, buffer, count+4, 0, &address, addressSize);
        if (sent == -1) {
            printf("Erreur lors de l'envoi du paquet %d.\n", blockNumber);
            return -1;
        }

        recvfrom(socketFd, aqc, ACQ_SIZE, 0, &address, &addressSize);
        if (aqc[0] == 0 && aqc[1] == 4 && (aqc[2] << 8) + aqc[3] == blockNumber) {
            printf("Paquet %d envoyé.\n", blockNumber);
        }

        blockNumber++;
    }
    return 0;
}

/**
 * Affiche un tableau d'octets en notatation binaire. Utile pour le debug.
 * 
 * @param str Le tableau.
 * @param length Sa longueur.
 */
void printStringAsBytes(char* str, int length) {
    for (int j=0; j<length; j++) {
        char a = str[j];
        for (int i=0; i<8; i++) {
            printf("%d", !!((a << i) & 0x80));
        }
        printf("\n");
    }
}