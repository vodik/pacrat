#define _GNU_SOURCE
#include "pacrat.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <alpm.h>

enum action {
    ACTION_LIST,
    ACTION_INVALID
};

const struct action_t *action = NULL;

const struct action_t *actions[ACTION_INVALID] = {
    [ACTION_LIST] = &list_action
};

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
        file_init(&b->system, backup->name, hash);

        /* snprintf(path, PATH_MAX, "%s/%s", pkgname, backup->name); */
        backups = alpm_list_add(backups, b);
    }

    return backups;
}

static alpm_list_t *find_all_modified(alpm_db_t *db)
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

static void backup_free(void *ptr)
{
    backup_t *b = ptr;

    free(b->system.path);
    free(b->system.hash);
    /* free(b->local.path); */
    /* free(b->local.hash); */
    free(b);
}

static void __attribute((__noreturn__)) usage(FILE *out)
{
    if (action && action->usage)
        action->usage(stdout);
    else {
        fprintf(out, "usage: %s [options] <command>...\n", program_invocation_short_name);
        fputs("\nOptions:\n", out);
        fputs("  -h, --help  display this help and exit\n", out);

        fputs("\nCommands:\n", out);
        fputs("  list        list all modified config files\n", out);
        fputs("  pull        make local copy\n", out);
    }

    exit(out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

static const struct action_t *string_to_action(const char *string)
{
    size_t i;

    for (i = 0; i < ACTION_INVALID; i++)
        if (strcmp(actions[i]->name, string) == 0)
            return actions[i];

    return NULL;
}

void parseargs(int argc, char *argv[])
{
    int index = 0;
    bool help = false;

    static const char *optstring = "har:";
    static const struct option opts[] = {
        { "help", no_argument,       0, 'h' },
        { "all",  no_argument,       0, 'a' },
        { "root", required_argument, 0, 'r' },
        { 0, 0, 0, 0 },
    };

    for (;;) {
        int opt = getopt_long(argc, argv, optstring, opts, &index);
        if (opt == -1)
            break;

        switch (opt) {
            case 'h':
                usage(stdout);
                break;
            default:
                break;
        }
    }

    action = optind == argc ? NULL : string_to_action(argv[optind]);

    if (help)
        usage(stdout);

    if (action == NULL) {
        if (optind == argc)
            usage(stderr);
        else
            errx(EXIT_FAILURE, "unknown action: %s", argv[optind]);
    }

    if (action->parsearg) {
        optind = 0;
        for (;;) {
            int opt = getopt_long(argc, argv, optstring, opts, &index);
            if (opt == -1)
                break;
            action->parsearg(opt);
        }
    }
}

int main(int argc, char *argv[])
{
    alpm_errno_t alpmerr;
    alpm_handle_t *alpm;
    alpm_db_t *db;
    alpm_list_t *modified;
    int rc = 0;

    parseargs(argc, argv);

    if (chdir("/") != 0)
        err(EXIT_FAILURE, "failed to chdir to /");

    alpm = alpm_initialize("/", "/var/lib/pacman", &alpmerr);
    if (alpm == NULL)
        err(EXIT_FAILURE, "failed to initialize alpm: %s", alpm_strerror(alpmerr));

    db = alpm_get_localdb(alpm);

    modified = find_all_modified(db);
    rc = action->cmd(modified);

    alpm_list_free_inner(modified, backup_free);
    alpm_list_free(modified);
    alpm_release(alpm);
    return rc;
}

// vim: et:sts=4:sw=4:cino=(0
