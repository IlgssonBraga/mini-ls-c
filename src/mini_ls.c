#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void usage(const char *p) {
    fprintf(stderr, "uso: %s [-a] [-l] [diretorio]\n", p);
}
static void mode_to_string(mode_t m, char out[11]) {
    out[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : '-';
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

int main(int argc, char **argv) {
    bool show_all = false, long_fmt = false;
    int opt;
    while ((opt = getopt(argc, argv, "alh")) != -1) {
        switch (opt) {
            case 'a': show_all = true; break;
            case 'l': long_fmt = true; break;
            case 'h': usage(argv[0]); return 0;
            default: usage(argv[0]); return 1;
        }
    }
    const char *target = (optind < argc) ? argv[optind] : ".";
    DIR *dir = opendir(target);
    if (!dir) {
        fprintf(stderr, "erro: não foi possível abrir '%s': %s\n", target, strerror(errno));
        return 1;
    }

    struct dirent *ent;
    char path[4096];
    while ((ent = readdir(dir)) != NULL) {
        if (!show_all && ent->d_name[0] == '.') continue;
        if (!long_fmt) {
            puts(ent->d_name);
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", target, ent->d_name);
        struct stat st;
        if (lstat(path, &st) != 0) {
            printf("?????????? %10s %s\n", "?", ent->d_name);
            continue;
        }
        char mode[11]; mode_to_string(st.st_mode, mode);
        printf("%s %8ld %s\n", mode, (long)st.st_size, ent->d_name);
    }
    closedir(dir);
    return 0;
}
