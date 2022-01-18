#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

const int MAX_STRING_LEN = 1024;
const int MAX_PIPED_CMDS = 128;

const char* WELCOME_MESSAGE = "  ____  _                                      _ \n |  _ \\(_)                                    | |\n | |_) |_  ___ _ ____   _____ _ __  _   _  ___| |\n |  _ <| |/ _ \\ '_ \\ \\ / / _ \\ '_ \\| | | |/ _ \\ |\n | |_) | |  __/ | | \\ V /  __/ | | | |_| |  __/_|\n |____/|_|\\___|_| |_|\\_/ \\___|_| |_|\\__,_|\\___(_)\n\nPour quitter, tapez 'exit'.\n";

const char* BYE_MESSAGE = "Vous repartez déjà ?\n";

const char* PROMPT_START = "\033[32menseash\033[39m";
const char* PROMPT_END = " % ";

typedef enum ioDirection { INPUT, OUTPUT } IODir;

void printToConsole(char* str);

void setColorToRed();

void resetColors();

void printError(char* str);

void displayPrompt(int status, int showStatus, float timeDeltaMs);

int countChar(char* str, char chr);

int countSpaces(char* str);

int countPipes(char* str);

char** parseArguments(char* str);

char* checkForRedirection(char* cmd, IODir* dir);

void splitAtPipes(char* cmd, char** subCmds);

float launchAndMesureTime(char* cmd, int* status);

float launchWithArgsAndMesureTime(char* cmd, char** argv, int* status);

float launchProcess(char* cmd, char** argv, int* status, char* filename, IODir dir);

float launchPipedProcesses(char** subCmds, int numCmds, int* status, char* filename, IODir dir);