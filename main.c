#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>

#include <alpm.h>
#include "pacrat.h"
#include "actions.h"

const struct action_t *action = NULL;

void usage(FILE *out)
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

const action_t *string_to_action(const char *string)
{
    if (strcmp(string, "list") == 0)
        return &list_action;
    else if (strcmp(string, "pull") == 0)
        return &pull_action;

    return NULL;
}

void parseargs(int argc, char *argv[])
{
    int index = 0;
    bool help = false;

    static const char *optstring = "hr:";
    static const struct option opts[] = {
        { "help", no_argument,       0, 'h' },
        { "root", required_argument, 0, 'r' },
        { 0, 0, 0, 0 },
    };

    for (;;) {
        int opt = getopt_long(argc, argv, optstring, opts, &index);
        if (opt == -1)
            break;

        switch (opt) {
            case 'h':
                help = true;
                break;
            default:
                break;
        }
    }

    action = (optind == argc) ? NULL : string_to_action(argv[optind]);

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
    rc = action->action(modified);

    alpm_list_free_inner(modified, backup_free);
    alpm_list_free(modified);
    alpm_release(alpm);
    return rc;
}

// vim: et:sts=4:sw=4:cino=(0
