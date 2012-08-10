#define _GNU_SOURCE
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
#include "pacrat.h"
#include "actions.h"

static char *compute_hash(const char *path)
{
    char *hash = alpm_compute_md5sum(path);
    if (!hash)
        errx(EXIT_FAILURE, "failed to compute hash for %s", path);
    return hash;
}

static void file_init(file_t *file, const char *path, char *hash)
{
    file->hash = hash ? hash : compute_hash(path);
    file->path = strndup(path, PATH_MAX);
}

static alpm_list_t *find_pkg_modified(alpm_pkg_t *pkg, bool all)
{
    alpm_list_t *backups = NULL;
    const alpm_list_t *i;

    /* struct stat st; */
    char path[PATH_MAX];

    const char *pkgname = alpm_pkg_get_name(pkg);

    for (i = alpm_pkg_get_backup(pkg); i; i = i->next) {
        const alpm_backup_t *backup = i->data;

        snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);

        /* check if we can read the file */
        if (access(path, R_OK) != 0) {
            warnx("cannot access %s", path);
            continue;
        }

        /* compute a hash and see if it differs from whats on record */
        char *hash = compute_hash(path);
        if (!all && strcmp(backup->hash, hash) == 0) {
            free(hash);
            continue;
        }

        /* create an entry */
        backup_t *b = calloc(1, sizeof(backup_t));

        b->pkgname = pkgname;
        b->hash = backup->hash;
        file_init(&b->system, path, hash);

        /* snprintf(path, PATH_MAX, "%s/%s", pkgname, backup->name); */
        backups = alpm_list_add(backups, b);
    }

    return backups;
}

alpm_list_t *find_all_modified(alpm_db_t *db)
{
    alpm_list_t *backups = NULL;
    const alpm_list_t *i;

    /* alpm_list_t *targets = target ? alpm_db_search(db, target) : alpm_db_get_pkgcache(db); */
    alpm_list_t *targets = alpm_db_get_pkgcache(db);
    for (i = targets; i; i = i->next) {
        alpm_list_t *pkg_backups = find_pkg_modified(i->data, false);
        backups = alpm_list_join(backups, pkg_backups);
    }

    return backups;
}

void backup_free(void *ptr)
{
    backup_t *b = ptr;

    free(b->system.path);
    free(b->system.hash);
    /* free(b->local.path); */
    /* free(b->local.hash); */
    free(b);
}

// vim: et:sts=4:sw=4:cino=(0
