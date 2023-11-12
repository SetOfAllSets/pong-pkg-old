#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>
#include "verbs.c"

/*
string size is 68 because, if config dir is $HOME/.config/pong-pkg and the username is 32 chars long, the 
longest allowed at time of writing, then 45 chars should be enough to store the config dir. 68 is
chosen to allow longer config dir paths than $HOME/.config/pong-pkg in $XDG_CONFIG_HOME for users with a 32 char name.
*/

char* configDir = "/etc/pong-pkg";
char* repoDir = "/etc/pong-pkg/repo";
char usrConfigDir[68];
char usrRepoDir[73];

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

//check for config dir and contents, if incomplete, create
int initConfigDir(char* dir) {
    struct stat st;
    int dirStatus = stat(dir,&st);
    //error code 2 is ENOENT; directory does not exist
    char* repoDir = concat(dir, "/repo");
    if(dirStatus != 0) {
        if(errno == 2) {
            mkdir(dir, 0700);
        } else {
            printf("Error accessing config directory (%s). Error was not ENOENT. Error code was %i", dir, errno);
        }
    }
    dirStatus = stat(repoDir,&st);
    if(dirStatus != 0) {
        if(errno == 2) {
            mkdir(repoDir, 0700);
        } else {
            printf("Error accessing repo directory (%s). Error was not ENOENT. Error code was %i", repoDir, errno);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (getenv("XDG_CONFIG_HOME") != NULL) {
        strcpy(usrConfigDir, concat(getenv("XDG_CONFIG_HOME"), "/pong-pkg"));
    } else {
        strcpy(usrConfigDir, concat(getenv("HOME"), "/.config/pong-pkg"));
    }
    //check for config dir and contents, if incomplete, create
    strcpy(usrRepoDir, concat(usrConfigDir, "/repo"));
    initConfigDir(usrConfigDir);
    return 0;
}
