#include "status.h"

#include <stdio.h>
#include <string.h>
#include "pacrat.h"
#include "config.h"

static void print_status(backup_t *);

void print_status(backup_t *b) /* {{{ */
{
	printf("%s%s%s %s\n", colstr.pkg, b->pkgname, colstr.nc, b->system.path);
	if (!b->local.path) {
		printf("  file not locally tracked\n");
	} else if (!STREQ(b->system.hash, b->local.hash)) {
		printf("  %s hashes don't match!\n", colstr.warn);
		printf("     %s\n     %s\n", b->system.hash, b->local.hash);
	}
} /* }}} */

int cmd_status(int argc, char **argv)
{
	alpm_list_t *backups = alpm_all_backups(cfg.all);
	alpm_list_t *i;

	for (i = backups; i; i = i->next)
		print_status(i->data);

	alpm_list_free_inner(backups, free_backup);
	alpm_list_free(backups);

	return 0;
}
