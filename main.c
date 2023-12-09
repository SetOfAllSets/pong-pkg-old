#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


u_int8_t runScript(const char* filePath) {
    pid_t pid = fork();
    if(pid < -1) {
        return 1;
    } else if(pid == 0) {
        execl("/bin/env", "-i", "PATH=/bin:/usr/bin", "/bin/sh", "%s", filePath);
        //execl() should never return unless something has gone wrong
        //execl() sets errno if it fails so we should just use that for error reporting if this fails
        return 2;
    }
    return 0;
}

u_int8_t install(int argc, char* argv[]) {
    printf("%s%i", argv[1], argc);
    return 0;
}

int main (int argc, char* argv[]) {
    //Check if a verb was provided
    if(argc<=1) {
        fprintf(stderr, "Not enough arguments.\n");
        //Call function to display help message. Doesn't exist yet.
        return EXIT_FAILURE;
    }
    if(!strcmp(argv[1], "install")) {
        install(argc, argv);
    } else if(!strcmp(argv[1], "remove") || !strcmp(argv[1], "uninstall")) {

    } else if(!strcmp(argv[1], "update")) {

    } else if(!strcmp(argv[1], "help")) {
        //Call function to display help message. Doesn't exist yet.
    } else {
        fprintf(stderr, "Unrecognized argument: %s\n", argv[1]);
        //Call function to display help message. Doesn't exist yet.
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
