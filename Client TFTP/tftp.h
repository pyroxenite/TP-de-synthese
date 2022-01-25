#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

const char* USAGE = "Usage : <address[:port]> <put|get> <filename>\n";
const char* DEFAULT_PORT = "69";

#define BUFFER_SIZE 516 // 4 + 512
#define ACQ_SIZE 4


// UTILITY

char* extractPort(char* addr);


// GET

void getFile(char* filename, char* addr, char* port);

void sendReadRequest(char* filename, char* addr, char* port, struct addrinfo** servinfo,
                     int* localFd, int* socketFd);

void formulateReadRequest(char* filename, char** requestPacket, int* requestPacketLength);

int receiveAndAcknowledge(int localFd, int socketFd);


// PUT

void putFile(char* filename, char* addr, char* port);

void sendWriteRequest(char* filename, char* addr, char* port, struct addrinfo** servinfo,
                      int* localFd, int* socketFd);

void formulateWriteRequest(char* filename, char** requestPacket, int* requestPacketLength);

int sendAndWaitAcknowledgement(int localFd, int socketFd);


// DEBUG

void printStringAsBytes(char* str, int length);