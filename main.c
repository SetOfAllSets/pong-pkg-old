#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>
#include <bsd/string.h>
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

char* concat(char *s1, char *s2)
{
    char* result = s1;
    strlcpy(result, s1, strlen(s1) + strlen(s2) + 1);
    strlcat(result, s2, strlen(s1) + strlen(s2) + 1);
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
        strlcpy(usrConfigDir, concat(getenv("XDG_CONFIG_HOME"), "/pong-pkg"), strlen(usrConfigDir) + strlen(concat(getenv("XDG_CONFIG_HOME"), "/pong-pkg")) + 1);
    } else {
        strlcpy(usrConfigDir, concat(getenv("HOME"), "/.config/pong-pkg"), strlen(usrConfigDir) + strlen(concat(getenv("HOME"), "/.config/pong-pkg")) + 1);
    }
    strlcpy(usrRepoDir, concat(usrConfigDir, "/repo"), (strlen(concat(usrConfigDir, "/repo") + strlen(usrRepoDir) + 1)));
    //check for config dir and contents, if incomplete, create
    initConfigDir(usrConfigDir);
    if (argc <= 1) {
        printf("Not enough arguments");
    }
    return 0;
}
