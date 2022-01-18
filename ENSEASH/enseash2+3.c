#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const int MAX_STRING_LEN = 1024;

const char* WELCOME_MESSAGE = "Bienvenue dans le Shell ENSEA (aka ENSEASH).\nPour quitter, tapez 'exit'.\n";

const char* BYE_MESSAGE = "Vous repartez déjà ?\n";

const char* PROMPT = "enseash % ";

void printToConsole(char* str) {
    int ret = write(STDOUT_FILENO, str, strlen(str));

    if (ret == -1) {
        exit(EXIT_FAILURE);
    }
}

int main() {
    char cmd[MAX_STRING_LEN];

    printToConsole((char *) WELCOME_MESSAGE);

    while (1) {
        // Affichage du prompt
        printToConsole((char *) PROMPT);

        // Lecture de la ligne de commande
        read(STDIN_FILENO, cmd, MAX_STRING_LEN);

        if (cmd[0] == '\n') {
            continue;
        }

        // Exit ?
        if (strncmp(cmd, "exit", 4) == 0) {
            break;
        }

        *strchr(cmd, '\n') = '\0';

        if (strchr(cmd, ' ') != NULL) {
            printToConsole("L'execution de commandes complexes n'est pas implémentée pour l'instant.\n");
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            int ret = execlp(cmd, cmd, (char *)NULL);
            if (ret == -1) {
                printToConsole("Le processus n'a pas pu être lancé.");
                exit(EXIT_FAILURE);
            }
        } else if (pid == -1) {
            printToConsole("Un nouveau processus n'a pas pu être créé.");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
        }
    }

    printToConsole((char *) BYE_MESSAGE);

    exit(EXIT_SUCCESS);
    return 0;
}