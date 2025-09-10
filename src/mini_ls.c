#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char *name;
    char path[PATH_MAX];
    struct stat st;
    bool stat_ok;
} Entry;

static void usage(const char *prog) {
    fprintf(stderr, "uso: %s [-a] [-l] [diretorio]\n", prog);
}

static char filetype(mode_t m) {
    if (S_ISREG(m)) return '-';
    if (S_ISDIR(m)) return 'd';
    if (S_ISLNK(m)) return 'l';
    if (S_ISCHR(m)) return 'c';
    if (S_ISBLK(m)) return 'b';
    if (S_ISFIFO(m)) return 'p';
    if (S_ISSOCK(m)) return 's';
    return '?';
}

static void mode_to_string(mode_t m, char out[11]) {
    out[0] = filetype(m);
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
    if (m & S_ISUID) out[3] = (out[3]=='x') ? 's' : 'S';
    if (m & S_ISGID) out[6] = (out[6]=='x') ? 's' : 'S';
    if (m & S_ISVTX) out[9] = (out[9]=='x') ? 't' : 'T';
}

static int cmp_entries(const void *a, const void *b) {
    const Entry *ea = (const Entry*)a, *eb = (const Entry*)b;
    return strcoll(ea->name, eb->name);
}

static void print_long(const Entry *e) {
    char mode[11]; mode_to_string(e->st.st_mode, mode);
    struct passwd *pw = getpwuid(e->st.st_uid);
    struct group  *gr = getgrgid(e->st.st_gid);
    const char *user = pw ? pw->pw_name : "?";
    const char *group = gr ? gr->gr_name : "?";

    char timebuf[64];
    struct tm lt;
    localtime_r(&e->st.st_mtime, &lt);
    time_t now = time(NULL);
    const char *fmt = ((now - e->st.st_mtime) > 15552000 || (e->st.st_mtime - now) > 15552000)
                      ? "%b %e  %Y" : "%b %e %H:%M";
    strftime(timebuf, sizeof timebuf, fmt, &lt);

    printf("%s %2lu %-8s %-8s %8ld %s %s",
           mode,
           (unsigned long)e->st.st_nlink,
           user, group,
           (long)e->st.st_size,
           timebuf,
           e->name);
    if (S_ISLNK(e->st.st_mode)) {
        char target[PATH_MAX];
        ssize_t n = readlink(e->path, target, sizeof(target)-1);
        if (n >= 0) {
            target[n] = '\0';
            printf(" -> %s", target);
        }
    }
    putchar('\n');
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

    struct stat st;
    if (lstat(target, &st) == 0 && !S_ISDIR(st.st_mode)) {
        Entry e = {0};
        e.name = (char*)target;
        strncpy(e.path, target, sizeof(e.path)-1);
        e.st = st;
        e.stat_ok = true;
        if (long_fmt) print_long(&e);
        else puts(e.name);
        return 0;
    }

    DIR *dir = opendir(target);
    if (!dir) {
        fprintf(stderr, "erro: não foi possível abrir '%s': %s\n", target, strerror(errno));
        return 1;
    }

    size_t cap = 64, n = 0;
    Entry *items = malloc(cap * sizeof *items);
    if (!items) { perror("malloc"); closedir(dir); return 1; }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (!show_all && name[0] == '.') continue;
        if (n == cap) {
            cap *= 2;
            Entry *tmp = realloc(items, cap * sizeof *items);
            if (!tmp) { perror("realloc"); free(items); closedir(dir); return 1; }
            items = tmp;
        }
        items[n].name = strdup(name);
        if (!items[n].name) { perror("strdup"); free(items); closedir(dir); return 1; }
        if (snprintf(items[n].path, sizeof(items[n].path), "%s/%s", target, name) >= (int)sizeof(items[n].path)) {
            fprintf(stderr, "caminho muito longo: %s/%s\n", target, name);
            free(items[n].name);
            continue;
        }
        if (lstat(items[n].path, &items[n].st) == 0) items[n].stat_ok = true;
        else items[n].stat_ok = false;
        n++;
    }
    closedir(dir);

    qsort(items, n, sizeof(Entry), cmp_entries);

    for (size_t i = 0; i < n; i++) {
        if (long_fmt && items[i].stat_ok) print_long(&items[i]);
        else puts(items[i].name);
        free(items[i].name);
    }
    free(items);
    return 0;
}