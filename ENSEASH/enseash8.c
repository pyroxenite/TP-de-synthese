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

/**
 * Recherche un éventuel chevron et découpe la commande si besoin.
 * 
 * @param cmd La commande complète
 * @param dir Lecture avec '<' ou écriture avec '>'. Prend ses valeurs dans l'enum IODir
 * @return char* Le nom du fichier ou un pointeur vers NULL.
 */
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

/**
 * Sépare la commande en sous-commandes renseignées dans `subCmds`. Élimine les espaces
 * superflus qui pourraient poser parfois problème.
 * 
 * @param cmd La commande complète mais sans redirections
 * @param subCmds Les sous-commandes
 */
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

/**
 * Lance toutes les commandes séparées par de pipe une par une en écrivant 
 * les données inteermédiares dans un fichier temporaraire.
 * 
 * @param subCmds Le tableau des sous-commandes.
 * @param numCmds La longueur du tableau de commandes.
 * @param status Pointeur vers status pour pouvoir le mettre à jour.
 * @param filename Un fichier en entrée ou en sortie (les deux ne peuvent pas 
 * être présents simultanément pour le moment...)
 * @param dir Permet dee savoir s'il faut lire ou écrire
 * @return float Le temps total d'exection en ms.
 */
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

        // Nouveau process
        pid_t pid = fork();

        if (pid == 0) { // Fils
            // Gestion des pipe et redirections
            if (i < numCmds - 1) {
                // Overture d'un fichier temporaire pour les sortie des n-1 premières commandes
                remove(TEMP_FILE);
                int fd = open(TEMP_FILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, STDOUT_FILENO);
            } else if (filename != NULL) {
                // Geestion des redirections
                if (dir == OUTPUT) {
                    // cmd > fichier
                    remove(filename);
                    int fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                } else {
                    // cmd < fichier
                    int fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR);
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
            }
            if (i > 0) {
                // Overture du fichier temporaire en tant qu'entrée des n-1 dernières commandes
                int fd = open(TEMP_FILE, O_RDONLY, S_IRUSR | S_IWUSR);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // Lancement d'une commande
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

    // Efface le fichier temporaire s'il a été créé 
    remove(TEMP_FILE);

    // Fin du chronomètre
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1) {
        printError("Une erreur liée à la gestion du temps s'est produite.");
        exit(EXIT_FAILURE);
    }

    // Delta en ms
    return (stop.tv_sec - start.tv_sec)*1000 + (stop.tv_nsec - start.tv_nsec)/1.0e6;
}

/**
 * Découpe et néttoie les arguments d'une commande.
 * 
 * @param str La commande complexe
 * @return char** Les morceaux de commande 'argv'
 */
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

/**
 * Printf sans f
 * 
 * @param str La chaine à afficher
 */
void printToConsole(char* str) {
    int ret = write(STDOUT_FILENO, str, strlen(str));

    if (ret == -1) {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Afficher une erreur en rouge
 * 
 * @param str Le texte à afficher
 */
void printError(char* str) {
    printToConsole("\033[91m");
    printToConsole(str);
    printToConsole("\033[39m\n");
}

/**
 * Affichage du prompt avec signaux et temps d'execution.
 * 
 * @param status Informations liées à l'execution
 * @param showStatus Afficher ou non le status
 * @param timeDeltaMs Temps à afficher
 */
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

/**
 * Compte le nombre d'occurences d'un caractère dans un chaine.
 * 
 * @param str La chaine.
 * @param chr Le caractère.
 * @return int Le compte.
 */
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