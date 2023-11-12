#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>
#include <bsd/string.h>

//change these to  your liking
const int PACKAGE_NAME_MAX_LENGTH = 256;
#define USR_CONFIG_DIR_MAX_LENGTH 68

/*
string size is 68 because, if config dir is $HOME/.config/pong-pkg and the username is 32 chars long, the
longest allowed at time of writing, then 45 chars should be enough to store the config dir. 68 is
chosen to allow longer config dir paths than $HOME/.config/pong-pkg in $XDG_CONFIG_HOME for users with a 32 char name.
*/

int run = 1;
int exitCode = 0;
char *configDir = "/etc/pong-pkg";
char *repoDir = "/etc/pong-pkg/repo";
char usrConfigDir[USR_CONFIG_DIR_MAX_LENGTH];
char usrRepoDir[USR_CONFIG_DIR_MAX_LENGTH+5];

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

int removeDuplicates(int argc, char *argv[], char packages[argc-2][PACKAGE_NAME_MAX_LENGTH]) {
    char tempPackages[argc-2][PACKAGE_NAME_MAX_LENGTH];
    for(int i = 0; i<argc-2;i++) {
        strlcpy(tempPackages[i],argv[i+2],PACKAGE_NAME_MAX_LENGTH);
    }
    qsort(tempPackages, argc-2, PACKAGE_NAME_MAX_LENGTH, (int(*)(const void*,const void*))strcoll);
    u_int16_t offset = 0;
    for(int i = 0;i<argc-2;i++) {
        if(i+1<=argc-2 && strcmp(tempPackages[i], tempPackages[i+1]) != 0) {
            strlcpy(packages[i-offset],tempPackages[i], PACKAGE_NAME_MAX_LENGTH);
        } else if(strcmp(tempPackages[i], tempPackages[i+1]) == 0) {
            offset++;
        }
    }
    return argc-2-offset;
}
int main(int argc, char *argv[]) {
    do{
        if(getenv("XDG_CONFIG_HOME") != NULL) {
            if(concat(getenv("XDG_CONFIG_HOME"), "/pong-pkg", usrConfigDir, sizeof(usrConfigDir)) != 0) {break;};
        } else {
            if(concat(getenv("HOME"), "/.config/pong-pkg", usrConfigDir, sizeof(usrConfigDir)) != 0) {break;};
        }
        if(concat(usrConfigDir, "/repo", usrRepoDir, sizeof(usrRepoDir)) != 0) {break;};
        // check for config dir and contents, if incomplete, create
        if(initConfigDir(usrConfigDir) != 0) {break;};
        if(argc <= 1) {
            fprintf(stderr, "Not enough arguments\n");
            exitCode = -1;
            break;
        } else {
            if(strcmp(argv[1], "install") == 0 || strcmp(argv[1], "update") == 0 || strcmp(argv[1], "remove") == 0) {
                // run corresponding script (for sync that would be the update script for the repo package)
                if(argc >= 3) {
                    char packages[argc-2][PACKAGE_NAME_MAX_LENGTH];
                    removeDuplicates(argc, argv, packages);
                } else {
                    fprintf(stderr, "Not enough arguments for verb %s\n", argv[1]);
                    exitCode = -1;
                    break;
                }
            }
        }
    } while(0);
    return exitCode;
}
