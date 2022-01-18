#include "enseash.h"

int main() {
    char commandBuffer[MAX_STRING_LEN];
    int showStatus = 0;
    int status = 0;
    float timeDeltaMs = 0;
    char* filename = NULL;
    IODir redirectionDir;

    int numCmds;
    char* subCmds[MAX_PIPED_CMDS];

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
        if (strncmp(cmd, "exit", 4) == 0 || strlen(cmd) == 0)
            break;

        // Supprime le retour à la ligne
        *strchr(cmd, '\n') = '\0';

        // > < > < > < > < > <
        filename = checkForRedirection(cmd, &redirectionDir);

        // xxx | xxx | xxx
        numCmds = countPipes(cmd) + 1;
        splitAtPipes(cmd, subCmds);

        timeDeltaMs = launchPipedProcesses(subCmds, numCmds, &status, filename, redirectionDir);
    }

    printToConsole((char *) BYE_MESSAGE);

    exit(EXIT_SUCCESS);
    return 0;
}

char* checkForRedirection(char* cmd, IODir* dir) {
    char* filename = strchr(cmd, '>');
    if (filename != NULL) {
        *dir = OUTPUT;
    } else {
        filename = strchr(cmd, '<');
        if (filename != NULL) {
            *dir = INPUT;
        } else {
            // Pas de '>' ni de '<'
            return (char*) NULL;
        }
    }

    // Retire les espaces avant '>' ou '<' et découpe la chaine de caractère 
    char* ptr = filename - 1;
    while (*ptr == ' ')
        ptr--;

    *(ptr + 1) = '\0';

    // Retire les espaces après '>' ou '<'
    filename++;
    while (strncmp(filename, " ", 1) == 0)
        filename++;

    return filename;
}

void splitAtPipes(char* cmd, char** subCmds) {
    char* ptr = cmd;
    subCmds[0] = cmd;
    int count = 1;

    // Parcourt la chaine
    while (*ptr != '\0') {
        if (*ptr == '|') {
            // Enlève les espaces avant '|' et découpe la chaine
            char* ptr2 = ptr;
            while (*(ptr2 - 1) == ' ')
                ptr2--;
            *ptr2 = '\0';

            // Enlève les espaces après '|' et rensigne le début de chaine dans `subCmds`
            ptr++;
            while (*ptr == ' ')
                ptr++;
            subCmds[count] = ptr;
            count++;
        }
        ptr++;
    }
}

float launchPipedProcesses(char** subCmds, int numCmds, int* status, char* filename, IODir dir) {
    struct timespec start, stop;
    char* cmd;
    char** argv;

    // Lancement du chronomètre
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        printError("Une erreur liée à la gestion du temps s'est produite.");
        exit(EXIT_FAILURE);
    }

    for (int i=0; i<numCmds; i++) {
        cmd = subCmds[i];
        argv = parseArguments(cmd);
        int stdoutCopy = dup(STDOUT_FILENO);

        // Nouveau process
        pid_t pid = fork();

        if (pid == 0) { // Fils
            if (i < numCmds - 1) {
                dup2(STDIN_FILENO, STDOUT_FILENO);
            } else if (filename != NULL) {
                if (dir == OUTPUT) {
                    remove(filename);
                    int fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                    dup2(fd, STDOUT_FILENO);
                } else {
                    int fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR);
                    dup2(fd, STDIN_FILENO);
                }
            }

            int ret = execvp(cmd, argv);
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
    }

    // Fin du chronomètre
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
        printError("Une erreur liée à la gestion du temps s'est produite.");
        exit(EXIT_FAILURE);
    }

    return (stop.tv_sec - start.tv_sec)*1000 + (stop.tv_nsec - start.tv_nsec)/1.0e6;
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

int countChar(char* str, char chr) {
    int count = 0;
    for (int i = 0; i < (int) strlen(str); i++)
        count += (str[i] == chr);
    return count;
}

int countSpaces(char* str) {
    return countChar(str, ' ');
}

int countPipes(char* str) {
    return countChar(str, '|');
}