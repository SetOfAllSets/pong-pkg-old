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
        run = 0;
        return 1;
    }
    strlcpy(outputString, s1, strlen(s1) + strlen(s2) + 1);
    strlcat(outputString, s2, strlen(s1) + strlen(s2) + 1);
    return 0;
}

// check forconfig dir and contents, ifincomplete, create
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
                run = 0;
            }
        } else {
            fprintf(stderr, "Error accessing config directory (%s). Error was not ENOENT. Error was: %s Error number was: %i\n", dir, strerror(errno), errno);
            exitCode = errno;
            run = 0;
        }
    }
    dirStatus = stat(repoDirTmp, &st);
    if(dirStatus != 0) {
        if(errno == 2) {
            if(mkdir(repoDirTmp, 0700) != 0)
            {
                fprintf(stderr, "Could not create repo directory (%s). Error was: %s\n", repoDirTmp, strerror(errno));
                exitCode = errno;
                run = 0;
            }
        } else {
            fprintf(stderr, "Error accessing repo directory (%s). Error was not ENOENT. Error was: %s\n", repoDirTmp, strerror(errno));
            exitCode = errno;
            run = 0;
        }
    }
    return 0;
}

// run script for verb
int runScript(int argc, char *argv[]) {
    // remove duplicates
    // run all the scripts
    char packages[argc-2][PACKAGE_NAME_MAX_LENGTH];
    for(int i = 0; i<argc-2;i++) {
        strlcpy(packages[i],argv[i+2],PACKAGE_NAME_MAX_LENGTH);
    }
    qsort(packages, argc-2, PACKAGE_NAME_MAX_LENGTH, (int(*)(const void*,const void*))strcoll);
    for(int i = 0; i<argc-2;i++) {
        printf("%s\n", packages[i]);
    }
    return 0;
}
int main(int argc, char *argv[]) {
    while(run) {
        if(getenv("XDG_CONFIG_HOME") != NULL) {
            concat(getenv("XDG_CONFIG_HOME"), "/pong-pkg", usrConfigDir, sizeof(usrConfigDir));
        } else {
            concat(getenv("HOME"), "/.config/pong-pkg", usrConfigDir, sizeof(usrConfigDir));
        }
        concat(usrConfigDir, "/repo", usrRepoDir, sizeof(usrRepoDir));
        // check for config dir and contents, if incomplete, create
        initConfigDir(usrConfigDir);
        if(argc <= 1) {
            fprintf(stderr, "Not enough arguments\n");
            exitCode = -1;
            run = 0;
        } else {
            if(strcmp(argv[1], "install") == 0 || strcmp(argv[1], "update") == 0 || strcmp(argv[1], "remove") == 0) {
                // run corresponding script (for sync that would be the update script for the repo package)
                if(argc >= 3) {
                    runScript(argc, argv);
                } else {
                    fprintf(stderr, "Not enough arguments for verb %s\n", argv[1]);
                    exitCode = -1;
                    run = 0;
                }
            }
        }
        run = 0;
    }
    return exitCode;
}
