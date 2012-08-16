#define _GNU_SOURCE
#include "pacrat.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <alpm.h>

static void usage(FILE *out)
{
    fprintf(out, "usage: %s list [options] <command>...\n", program_invocation_short_name);
}

static int list(alpm_list_t *modified)
{
    const alpm_list_t *i;
    for (i = modified; i; i = i->next) {
        const backup_t *b = i->data;
        printf("%s :: %s [%s]\n", b->pkgname, b->system.path, b->system.hash);
    }

    return 0;
}

const struct action_t list_action = {
    .name     = "list",
    .parsearg = NULL,
    .cmd      = list,
    .usage    = usage
};

// vim: et:sts=4:sw=4:cino=(0
