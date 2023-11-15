#include <stdlib.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>
#include <bsd/string.h>
#include <pthread.h> 
#include <sys/wait.h>
#include <termios.h>
#include "defines.h"

/*
string size is 68 because, if config dir is $HOME/.config/pong-pkg and the username is 32 chars long, the
longest allowed at time of writing, then 45 chars should be enough to store the config dir. 68 is
chosen to allow longer config dir paths than $HOME/.config/pong-pkg in $XDG_CONFIG_HOME for users with a 32 char name.
*/

pthread_mutex_t packageProgressMutex;
u_int8_t packageProgress = 0;

struct arg_struct {
    char verb[32];
    u_int8_t packageCount;
    char packages[MAX_PACKAGES_PER_RUN][PACKAGE_NAME_MAX_LENGTH];
};

char configDir[] = CONFIG_DIR;
char repoDir[] = REPO_DIR;
u_int16_t packageNameMaxLength = PACKAGE_NAME_MAX_LENGTH;
u_int8_t maxThreads = MAX_THREADS;
struct arg_struct args;
u_int8_t boolPrompt(char message[], u_int8_t defaultValue) {
    // get ready to disable canonical mode
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &=(~ICANON );

    int exitCode = 0;
    printf("%s", message);
    // disable canonical mode
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
    char input = getc(stdin);
    // enable canonical mode again
    if(input != '\n') {
        printf("\n");
    }
    tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
    if(input != 'y' && input != 'n' && input != '\n') {
        printf("Invalid reponse.\n");
        boolPrompt(message, defaultValue);
    } else if(input == 'y') {
        exitCode = 1;
    } else if(input == 'n') {
        exitCode = 0;
    } else {
        exitCode = defaultValue;
    }
    return exitCode;
}

u_int8_t concat(char *s1, char *s2, char *outputString, size_t outputSize) {
    u_int8_t exitCode = 0;
    if(outputSize < strlen(s1) + strlen(s2) + 1) {
        fprintf(stderr, "Could not concatonate strings, size of buffer too small. %lu<%lu", outputSize, strlen(s1) + strlen(s2) + 1);
        exitCode = -1;
        return exitCode;
    }
    strlcpy(outputString, s1, strlen(s1) + strlen(s2) + 1);
    strlcat(outputString, s2, strlen(s1) + strlen(s2) + 1);
    return 0;
}

u_int8_t removeDuplicates(int argc, char *argv[], char packages[argc-2][packageNameMaxLength]) {
    char tempPackages[argc-2][packageNameMaxLength];
    for(int i = 0; i<argc-2;i++) {
        strlcpy(tempPackages[i],argv[i+2],packageNameMaxLength);
    }
    qsort(tempPackages, argc-2, packageNameMaxLength, (int(*)(const void*,const void*))strcoll);
    u_int16_t offset = 0;
    u_int8_t packageCount = 0;
    for(int i = 0;i<argc-2;i++) {
        if(i+1<=argc-2 && strcmp(tempPackages[i], tempPackages[i+1]) != 0) {
            strlcpy(packages[i-offset],tempPackages[i], packageNameMaxLength);
        } else if(strcmp(tempPackages[i], tempPackages[i+1]) == 0) {
            offset++;
        }
        packageCount = i-offset;
    }
    return packageCount;
}

void *runScript(void* argsIn) {
    u_int8_t currentPackage;
    struct arg_struct args = *(struct arg_struct*)argsIn;
    char dir[strlen(REPO_DIR)+PACKAGE_NAME_MAX_LENGTH+MAX_SCRIPT_NAME_LENGTH];
    while(1) {
        pthread_mutex_lock(&packageProgressMutex);
        if(packageProgress>args.packageCount) {
            pthread_mutex_unlock(&packageProgressMutex);
            break;
        }
        currentPackage = packageProgress;
        packageProgress++;
        pthread_mutex_unlock(&packageProgressMutex);
        strlcpy(dir, repoDir, sizeof(dir));
        strlcat(dir, args.packages[currentPackage], sizeof(dir));
        strlcat(dir, "/", sizeof(dir));
        strlcat(dir, args.verb, sizeof(dir));
        strlcat(dir, ".sh", sizeof(dir));
        pid_t pid = fork();
        int exitStatus = -1 ;
        if(pid == 0){
            execl("/bin/sh", "-c", dir, NULL);
            // next part should never run as execl replaces the current program, if it runs, execl has returned, indicating an error
            fprintf(stderr, "Failed to run shell script %s for package %s, execl failed with error: %s, error code %i\n", dir, args.packages[currentPackage], strerror(errno), errno);
            exit(-1);
        } else if(pid<0) {
            fprintf(stderr, "Failed to fork in order to run script.\n");
        } else {
            wait(&exitStatus);
        }
        if(exitStatus != 0) {
            fprintf(stderr, "Error operating on package %s. The error occured when running one of the package's scripts (%s). The error code was: %i. The error was: %s\n",args.packages[currentPackage], dir, exitStatus, strerror(exitStatus));
        }
    }
    return NULL;
}

u_int8_t runVerb(u_int8_t packageCount, char packages[packageCount][packageNameMaxLength], char verb[]) {
    pthread_t threads[maxThreads-1];
    strlcpy(args.verb, verb, sizeof(args.verb));
    args.packageCount = packageCount;
    for(int i = 0; i<=packageCount; i++) {
        strlcpy(args.packages[i], packages[i], sizeof(args.packages[i]));
    }
    for(int i = 0; i<packageCount-1 && i<maxThreads-1; i++) {
        pthread_create(&threads[i], NULL, runScript, &args);
    }
    runScript(&args);
    for(int i = 0; i<packageCount-1 && i<maxThreads-1; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

u_int8_t sanitizeInput(u_int8_t packageCount, char packages[packageCount][packageNameMaxLength]) {
    u_int8_t exitCode = 0;
    for(int i = 0; i<=packageCount; i++) {
        for(int j = 0; j<strlen(packages[i])-2; j++) {
            if(packages[i][j] == '/' && packages[i][j+1] == '.' && packages[i][j+2] == '.' && packages[i][j+3] == '/') {
                exitCode = 1;
                break;
            }
        }
    }
    return exitCode;
}

int main(int argc, char *argv[]) {
    if(strcmp(getenv("USER"), "root") != 0) {
        u_int8_t continueAsNonRoot = boolPrompt("Not running as root. Continue anyway? [y/N] ", 0);
        if(continueAsNonRoot == 0) {
            exit(0);
        }
    }
    if(argc <= 1) {
        fprintf(stderr, "Not enough arguments\n");
        exit(-1);
    } else {
        if(strcmp(argv[1], "install") == 0 || strcmp(argv[1], "update") == 0 || strcmp(argv[1], "remove") == 0 || strcmp(argv[1], "sync") == 0) {
            // run corresponding script (for sync that would be the update script for the repo package)
            if(argc >= 3) {
                char packages[argc-2][packageNameMaxLength];
                u_int8_t packageCount = removeDuplicates(argc, argv, packages);
                if(sanitizeInput(packageCount, packages) != 0) {
                    fprintf(stderr, "\"/../\" not allowed in package names\n");
                    exit(-1);
                }
                runVerb(packageCount, packages, argv[1]);
            } else {
                fprintf(stderr, "Not enough arguments for verb %s\n", argv[1]);
                exit(-1);
            }
        } else {
            fprintf(stderr, "Unkown verb\n");
        }
    }
    return 0;
}
