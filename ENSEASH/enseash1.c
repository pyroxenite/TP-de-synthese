#include <stdio.h>
#include <string.h>
#include <limits.h>

const char* WELCOME_MESSAGE = "Bienvenue dans le Shell ENSEA (aka ENSEASH).\nPour quitter, tapez 'exit'.\n";

void showWelcomeMessage() {
    fwrite(WELCOME_MESSAGE, sizeof(char), strlen(WELCOME_MESSAGE), stdout);
}

int main() {
    showWelcomeMessage();

    return 0;
}