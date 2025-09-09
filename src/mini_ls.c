#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *target = (argc > 1) ? argv[1] : ".";
    DIR *dir = opendir(target);
    if (!dir) {
        fprintf(stderr, "erro: não foi possível abrir '%s': %s\n", target, strerror(errno));
        return 1;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        puts(ent->d_name);
    }
    closedir(dir);
    return 0;
}