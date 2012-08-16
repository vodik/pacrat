#define _GNU_SOURCE
#include "pacrat.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <alpm.h>

#define OPEN(fd, path, flags) do { fd = open(path, flags); } while(fd == -1 && errno == EINTR)
#define CLOSE(fd) do { int _ret; do { _ret = close(fd); } while(_ret == -1 && errno == EINTR); } while(0)

const char *basedir;

static void usage(FILE *out)
{
    fprintf(out, "usage: %s pull [options] <command>...\n", program_invocation_short_name);
    fputs("\nOptions:\n", out);
    fputs("  -r, --root <path>  set the root path for pacrat\n", out);
}

static void parsearg(char opt)
{
    switch (opt) {
        case 'r':
            basedir = optarg;
            break;
    }
}

static int copyfile(const char *src, const char *dest, uid_t uid, gid_t gid)
{
    char buf[BUFSIZ];
    int in, out, ret = 1;
    ssize_t nread;
    struct stat st;

    fprintf(stderr, "copy %s -> %s\n", src, dest);

    OPEN(in, src, O_RDONLY);
    do { out = open(dest, O_WRONLY | O_CREAT, 0000); } while (out == -1 && errno == EINTR);

    if (in < 0 || out < 0)
        goto cleanup;

    if (fstat(in, &st) || fchmod(out, st.st_mode))
        goto cleanup;

    /* this might fail for non-root, but who cares */
    fchown(out, uid, gid);

    /* do the actual file copy */
    for (;;) {
        ssize_t nwrite = 0;
        nread = read(in, buf, BUFSIZ);

        if (nread == 0 || (nread < 0 && errno != EINTR))
            break;

        if(nread < 0)
            continue;

        do {
            nwrite = write(out, buf + nwrite, nread);
            if(nwrite >= 0)
                nread -= nwrite;
            else if(errno != EINTR)
                goto cleanup;
        } while (nread > 0);
    }
    ret = 0;

cleanup:
    if (in >= 0)
        CLOSE(in);
    if (out >= 0)
        CLOSE(out);

    return ret;
}

static int mkdir_parents(const char *path, mode_t mode) {
    struct stat st;
    const char *p, *e;

    /* return immediately if directory exists */
    e = strrchr(path, '/');
    if (!e)
        return -EINVAL;
    p = strndupa(path, e - path);
    if (stat(p, &st) >= 0) {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            return 0;
        else
            return -ENOTDIR;
    }

    /* create every parent directory in the path, except the last component */
    p = path + strspn(path, "/");
    for (;;) {
        int r;
        char *t;

        e = p + strcspn(p, "/");
        p = e + strspn(e, "/");

        /* is this the last component? if so, then we're done */
        if (*p == 0)
            return 0;

        t = strndup(path, e - path);
        if (!t)
            return -ENOMEM;

        r = mkdir(t, mode) < 0 ? -errno : 0;
        free(t);

        if (r < 0 && errno != EEXIST)
            return -errno;
    }

    return 0;
}

static int pull(alpm_list_t *modified)
{
    const alpm_list_t *i;

    if (basedir == NULL) {
        warnx("basedir is null");
        return 1;
    }

    for (i = modified; i; i = i->next) {
        char dest[PATH_MAX];
        /* char *hash; */
        struct stat st;
        const backup_t *b = i->data;

        if (stat(b->system.path, &st) < 0) {
            warn("failed to stat backup file: %s", b->system.path);
            continue;
        }

        snprintf(dest, PATH_MAX, "%s/%s%s", basedir, b->pkgname, b->system.path);

        if (mkdir_parents(dest, 0755) != 0) {
            warn("failed to create directory tree for %s", b->system.path);
            continue;
        }

        /* get the hash of the dest file, should it exist */
        /* hash = compute_hash(dest); */
        /* if (strcmp(hash, b->system.hash) != 0) { */
            if (copyfile(b->system.path, dest, st.st_uid, st.st_gid) != 0) {
                warn("failed to copy %s to %s", b->system.path, dest);
                continue;
            }
            /* b->updated = true; */
        /* } */
    }

    return 0;
}

const struct action_t pull_action = {
    .name     = "pull",
    .parsearg = parsearg,
    .cmd      = pull,
    .usage    = usage
};

// vim: et:sts=4:sw=4:cino=(0
