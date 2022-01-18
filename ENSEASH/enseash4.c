#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const int MAX_STRING_LEN = 1024;

const char* WELCOME_MESSAGE = "  ____  _                                      _ \n |  _ \\(_)                                    | |\n | |_) |_  ___ _ ____   _____ _ __  _   _  ___| |\n |  _ <| |/ _ \\ '_ \\ \\ / / _ \\ '_ \\| | | |/ _ \\ |\n | |_) | |  __/ | | \\ V /  __/ | | | |_| |  __/_|\n |____/|_|\\___|_| |_|\\_/ \\___|_| |_|\\__,_|\\___(_)\n\nPour quitter, tapez 'exit'.\n";

const char* BYE_MESSAGE = "Vous repartez déjà ?\n";

const char* PROMPT_START = "\033[32menseash\033[39m";
const char* PROMPT_END = " % ";

void printToConsole(char* str) {
    int ret = write(STDOUT_FILENO, str, strlen(str));

    if (ret == -1) {
        exit(EXIT_FAILURE);
    }
}

void setColorToRed() {
    printToConsole("\033[91m");
}

void resetColors() {
    printToConsole("\033[39m");
}

void mainloop() {
    char cmd[MAX_STRING_LEN];
    char printBuffer[20];
    int showStatus = 0;
    int status;

    while (1) {
        // Affichage du prompt
        printToConsole((char *) PROMPT_START);
        if (showStatus) {
            if (WIFSIGNALED(status)) {
                sprintf(printBuffer, " [sign:%d]", WTERMSIG(status));
                printToConsole(printBuffer);
            } else if (WIFEXITED(status)) {
                sprintf(printBuffer, " [exit:%d]", WEXITSTATUS(status));
                printToConsole(printBuffer);
            }
        }
        printToConsole((char *) PROMPT_END);

        // Lecture de la ligne de commande
        read(STDIN_FILENO, cmd, MAX_STRING_LEN);

        if (cmd[0] == '\n') {
            showStatus = 0;
            continue;
        } else {
            showStatus = 1;
        }

        // Exit ?
        if (strncmp(cmd, "exit", 4) == 0) {
            break;
        }

        *strchr(cmd, '\n') = '\0';

        if (strchr(cmd, ' ') != NULL) {
            setColorToRed();
            printToConsole("L'execution de commandes complexes n'est pas implémentée pour l'instant.\n");
            resetColors();
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            int ret = execlp(cmd, cmd, (char *)NULL);
            if (ret == -1) {
                setColorToRed();
                printToConsole("Le processus n'a pas pu être lancé.\n");
                resetColors();
                exit(EXIT_FAILURE);
            }
        } else if (pid == -1) {
            setColorToRed();
            printToConsole("Un nouveau processus n'a pas pu être créé.\n");
            resetColors();
            exit(EXIT_FAILURE);
        } else {
            wait(&status);
        }
    }

    printToConsole((char *) BYE_MESSAGE);

    exit(EXIT_SUCCESS);
}

int main() {
    printToConsole((char *) WELCOME_MESSAGE);

    mainloop();

    return 0;
}