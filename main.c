#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>
#include <bsd/string.h>
#include <pthread.h> 

//change these to  your liking
#define PACKAGE_NAME_MAX_LENGTH 256
#define MAX_THREADS 7
#define CONFIG_DIR "/etc/pong-pkg"
#define REPO_DIR "/etc/pong-pkg/repo"

/*
string size is 68 because, if config dir is $HOME/.config/pong-pkg and the username is 32 chars long, the
longest allowed at time of writing, then 45 chars should be enough to store the config dir. 68 is
chosen to allow longer config dir paths than $HOME/.config/pong-pkg in $XDG_CONFIG_HOME for users with a 32 char name.
*/

pthread_mutex_t packageProgressMutex;
u_int8_t packageProgress = 0;
int exitCode = 0;

struct arg_struct {
    char verb[32];
    u_int8_t packageCount;
    char packages[255][PACKAGE_NAME_MAX_LENGTH];
};

char configDir[] = CONFIG_DIR;
char repoDir[] = REPO_DIR;
u_int16_t packageNameMaxLength = PACKAGE_NAME_MAX_LENGTH;
u_int8_t maxThreads = MAX_THREADS;
struct arg_struct args;
u_int8_t boolPrompt(char message[], char input[1024]) {
    int exitCode = 0;
    printf("%s", message);
    //scanf("%s", input);
    fgets(input, 1024, stdin);
    if(strcmp(input,"y\n") != 0 && strcmp(input,"n\n") != 0 && strcmp(input,"\n") != 0) {
        printf("Invalid reponse.\n");
        boolPrompt(message, input);
    } else if(strcmp(input, "y\n") == 0) {
        exitCode = 0;
    } else {
        exitCode = 1;
    }
    return exitCode;
}

int concat(char *s1, char *s2, char *outputString, size_t outputSize) {
    if(outputSize < strlen(s1) + strlen(s2) + 1) {
        fprintf(stderr, "Could not concatonate strings, size of buffer too small. %lu<%lu", outputSize, strlen(s1) + strlen(s2) + 1);
        exitCode = -1;
        return 1;
    }
    strlcpy(outputString, s1, strlen(s1) + strlen(s2) + 1);
    strlcat(outputString, s2, strlen(s1) + strlen(s2) + 1);
    return 0;
}

// check for config dir and contents, ifincomplete, create
int initConfigDir(char *dir) {
    char repoDirTmp[strlen(dir) + 6];
    concat(dir, "/repo", repoDirTmp, sizeof(repoDirTmp));
    struct stat st;
    int dirStatus = stat(dir, &st);
    if(dirStatus != 0) {
        // error code 2 is ENOENT; directory does not exist
        if(errno == 2) {
            errno = 0;
            if(mkdir(dir, 0700) != 0) {
                fprintf(stderr, "Could not create config directory (%s). Error was: %s Error number was: %i\n", dir, strerror(errno), errno);
                exitCode = errno;
            }
        } else {
            fprintf(stderr, "Error accessing config directory (%s). Error was not ENOENT. Error was: %s Error number was: %i\n", dir, strerror(errno), errno);
            exitCode = errno;
        }
    }
    dirStatus = stat(repoDirTmp, &st);
    if(dirStatus != 0) {
        if(errno == 2) {
            if(mkdir(repoDirTmp, 0700) != 0) {
                fprintf(stderr, "Could not create repo directory (%s). Error was: %s\n", repoDirTmp, strerror(errno));
                exitCode = errno;
            }
        } else {
            fprintf(stderr, "Error accessing repo directory (%s). Error was not ENOENT. Error was: %s\n", repoDirTmp, strerror(errno));
            exitCode = errno;
        }
    }
    return 0;
}

u_int8_t removeDuplicates(int argc, char *argv[], char packages[argc-2][packageNameMaxLength]) {
    char tempPackages[argc-2][packageNameMaxLength];
    for(int i = 0; i<argc-2;i++) {
        strlcpy(tempPackages[i],argv[i+2],packageNameMaxLength);
    }
    qsort(tempPackages, argc-2, packageNameMaxLength, (int(*)(const void*,const void*))strcoll);
    u_int16_t offset = 0;
    u_int8_t packageCount;
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
    //u_int8_t currentPackage;
    struct arg_struct args = *(struct arg_struct*)argsIn;
    while(1) {
        pthread_mutex_lock(&packageProgressMutex);
        if(packageProgress>args.packageCount) {
            pthread_mutex_unlock(&packageProgressMutex);
            break;
        }
        printf("%s\n", args.packages[packageProgress]);
        packageProgress++;
        pthread_mutex_unlock(&packageProgressMutex);
    }
    return NULL;
}

int runVerb(u_int8_t packageCount, char packages[packageCount][packageNameMaxLength], char verb[]) {
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

int main(int argc, char *argv[]) {
    do{
        if(strcmp(getenv("USER"), "root") != 0) {
            char input[1024];
            u_int8_t continueAsNonRoot = boolPrompt("Not running as root. Continue anyway? [y/N]  ", input);
            if(continueAsNonRoot == 1) {
                break;
            }
        }
        if(argc <= 1) {
            fprintf(stderr, "Not enough arguments\n");
            exitCode = -1;
            break;
        } else {
            if(strcmp(argv[1], "install") == 0 || strcmp(argv[1], "update") == 0 || strcmp(argv[1], "remove") == 0 || strcmp(argv[1], "sync") == 0) {
                // run corresponding script (for sync that would be the update script for the repo package)
                if(argc >= 3) {
                    char packages[argc-2][packageNameMaxLength];
                    runVerb(removeDuplicates(argc, argv, packages), packages, argv[1]);
                } else {
                    fprintf(stderr, "Not enough arguments for verb %s\n", argv[1]);
                    exitCode = -1;
                    break;
                }
            } else {
                fprintf(stderr, "Unkown verb\n");
            }
        }
    } while(0);
    return exitCode;
}
