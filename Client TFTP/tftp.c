#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

const char* DEFAULT_PORT = "69";

#define BUFFER_SIZE 512 
#define ACQ_SIZE 4

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

void printStringAsBytes(char* str, int length) {
    for (int j=0; j<length; j++) {
        char a = str[j];
        for (int i=0; i<8; i++) {
            printf("%d", !!((a << i) & 0x80));
        }
        printf("\n");
    }
}

void formulateRequest(char* filename, char** requestPacket, int* requestPacketLength) {
    *requestPacketLength = 2 + strlen(filename) + 1 + strlen("octet") + 1;
    *requestPacket = (char*) malloc(*requestPacketLength * sizeof(char));

    // Opcode
    (*requestPacket)[0] = 0;
    (*requestPacket)[1] = 1;

    // Filename
    strcpy(*requestPacket + 2, filename);

    // Mode
    strcpy(*requestPacket + 2 + strlen(filename) + 1, "octet");
}

void formulateAcknowledge(char** acq, char* packet) {
    
}

int get(char* filename, char* addr, char* port) {
    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo* servinfo;

    printf("Demande de %s sur %s:%s...\n", filename, addr, port);

    if (getaddrinfo(addr, port, &hints, &servinfo)) {
        printf("Erreur : serveur inconnu.\n");
        return 1;
    }

    int local_fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRUSR|S_IRGRP|S_IROTH);
    int socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    char* requestPacket;
    int requestPacketLength;
    formulateRequest(filename, &requestPacket, &requestPacketLength);

    struct sockaddr adresse;
    socklen_t adresseSize = sizeof(adresse);

    char buffer[BUFFER_SIZE];
    
    int sent = sendto(socket_fd, requestPacket, 4+strlen(filename)+strlen("octet"), 0, servinfo->ai_addr, servinfo->ai_addrlen);
	if (sent == -1){
		printf("Erreur : la demande n'a pu être effectuée.\n");
		exit(EXIT_FAILURE);
	}

    int blockNumber = 1;
    int count = BUFFER_SIZE;
    while (count == BUFFER_SIZE) {
        count = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, &adresse, &adresseSize);
        if (count == -1) {
            printf("Erreur : mauvaise reception du packet %d.\n", blockNumber);
            close(socket_fd);
            close(local_fd);
            exit(EXIT_FAILURE);
        }

        printf("Packet %d reçu. (%d octets)\n", blockNumber, count);

        char acq[ACQ_SIZE];

        formulateAcknowledge(*acq, buffer);

        if (buffer[0] == 0 && buffer[1] == 3) {
            acq[0] = 0;
            acq[1] = 4;
            acq[2] = buffer[2];
            acq[3] = buffer[3];
        
        int sendacq = sendto(socket_fd, acq, ACQ_SIZE, 0, &adresse, adresseSize);

        if (sendacq == -1){
                printf("Erreur dans le deuxième envoi de données (acquittement).\n");
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
        }

        if (buffer[0]==0 && buffer[1]==5) {
            printf("Erreur\n");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        if ((buffer[2]<<8) + buffer[3] == blockNumber) {
            write(local_fd, buffer+4, count-4);
            blockNumber++;
        }
    }

    printf("Le fichier %s a été téléchargé avec succes.\n", filename);
    close(socket_fd);
    close(local_fd);

    return 0;
}



int main(int argc, char const *argv[]) {
    if (argc != 4) {
        printf("Usage : <address[:port]> <put|get> <filename>\n");
        return 0;
    }
    char* addr = (char*) argv[1];
    char* port = extractPort((char*) argv[1]);
    char* cmd = (char*) argv[2];
    char* filename = (char*) argv[3];
    if (strcmp(cmd, "get") == 0) {
        get(filename, addr, port);
    } else if (strcmp(cmd, "put") == 0) {
        printf("Putting %s on %s:%s...\n",  filename, addr, port);
    } else {
        printf("Usage : <address[:port]> <put|get> <filename>\n");
    }
    return 0;
}
