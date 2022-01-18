#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "enseash.h"

const int MAX_STRING_LEN = 1024;

const char* WELCOME_MESSAGE = "  ____  _                                      _ \n |  _ \\(_)                                    | |\n | |_) |_  ___ _ ____   _____ _ __  _   _  ___| |\n |  _ <| |/ _ \\ '_ \\ \\ / / _ \\ '_ \\| | | |/ _ \\ |\n | |_) | |  __/ | | \\ V /  __/ | | | |_| |  __/_|\n |____/|_|\\___|_| |_|\\_/ \\___|_| |_|\\__,_|\\___(_)\n\nPour quitter, tapez 'exit'.\n";

const char* BYE_MESSAGE = "Vous repartez déjà ?\n";

const char* PROMPT_START = "\033[32menseash\033[39m";
const char* PROMPT_END = " % ";

int main() {
    char commandBuffer[MAX_STRING_LEN];
    int showStatus = 0;
    int status = 0;
    float timeDeltaMs = 0;
    char** argv;

    printToConsole((char *) WELCOME_MESSAGE);

    while (1) {
        // Affichage du prompt
        displayPrompt(status, showStatus, timeDeltaMs);

        // Lecture de la ligne de commande
        read(STDIN_FILENO, commandBuffer, MAX_STRING_LEN);

        // Ignore les espaces en début de ligne
        char* cmd = commandBuffer;
        while (*cmd == ' ')
            cmd++;

        // Ignore une ligne vide
        if (cmd[0] == '\n')
            continue;

        // Pour ne pas afficher de 'status' pour les lignes vides
        showStatus = (cmd[0] != '\n');

        // Exit ?
        if (strncmp(cmd, "exit", 4) == 0)
            break;

        // Supprime le retour à la ligne
        *strchr(cmd, '\n') = '\0';

        // Déccoupe la commande complexe
        argv = parseArguments(cmd);

        // Executer la commande en mesurant le temps
        timeDeltaMs = launchWithArgsAndMesureTime(cmd, argv, &status);
    }

    printToConsole((char *) BYE_MESSAGE);

    exit(EXIT_SUCCESS);
    return 0;
}

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
    printToConsole("\033[39m\n");
}

void printError(char* str) {
    setColorToRed();
    printToConsole(str);
    resetColors();
}

void displayPrompt(int status, int showStatus, float timeDeltaMs) {
    char printBuffer[20];

    printToConsole((char *) PROMPT_START);
    if (showStatus) {
        if (WIFSIGNALED(status)) {
            sprintf(printBuffer, " [sign:%d|%.1fms]", WTERMSIG(status), timeDeltaMs);
            printToConsole(printBuffer);
        } else if (WIFEXITED(status)) {
            sprintf(printBuffer, " [exit:%d|%.1fms]", WEXITSTATUS(status), timeDeltaMs);
            printToConsole(printBuffer);
        }
    }
    printToConsole((char *) PROMPT_END);
}

float launchWithArgsAndMesureTime(char* cmd, char** argv, int* status) {
    struct timespec start, stop;

    // Lancement du chronomètre
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        printError("Une erreur liée à la gestion du temps s'est produite.");
        exit(EXIT_FAILURE);
    }

    // Nouveau process
    pid_t pid = fork();

    if (pid == 0) { // Fils
        int ret = execvp(cmd, argv);;
        if (ret == -1) {
            printError("Le processus n'a pas pu être lancé.");
            exit(EXIT_FAILURE);
        }
    } else if (pid == -1) { // Fausse couche
        printError("Un nouveau processus n'a pas pu être créé.");
        exit(EXIT_FAILURE);
    } else { // Père
        wait(status);
    }

    // Fin du chronomètre
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
        exit(EXIT_FAILURE);
    }

    return (stop.tv_sec - start.tv_sec)*1000 + (stop.tv_nsec - start.tv_nsec)/1.0e6;
}

int countSpaces(char* str) {
	int count = 0;
	for(int i = 0; i < (int) strlen(str); i++)
		count += (str[i] == ' ')?1:0;
	return count;
}

char** parseArguments(char* str) {
	int numberOfSpaces = countSpaces(str);
	char** argv = (char **) malloc((numberOfSpaces + 2)*sizeof(char*));

	argv[0] = str;
	int argvPos = 1;

	for (int i=0; i<(int) strlen(str); i++) {
		if (str[i] == ' ') {
			argv[argvPos++] = str + i + 1;
			str[i] = '\0';
		}
	}
	argv[numberOfSpaces+1] = (char*) NULL;
	return argv;
}