#include "status.h"

#include <stdio.h>
#include "pacrat.h"

static void print_status(backup_t *);

void print_status(backup_t *b) /* {{{ */
{
	b = b;
	/* printf("%s %s%s%s %s\n", colstr->info, colstr->pkg, b->pkgname, colstr->nc, b->path); */
	/* printf("%s%s%s %s\n", colstr->pkg, b->pkgname, colstr->nc, b->system.path); */
	/* if (!b->local.path) { */
	/* 	printf("  file not locally tracked\n"); */
	/* } else if (!STREQ(b->system.hash, b->local.hash)) { */
	/* 	printf("  %s hashes don't match!\n", colstr->warn); */
	/* 	printf("     %s\n     %s\n", b->system.hash, b->local.hash); */
	/* } */
} /* }}} */

int cmd_status(int argc, char **argv)
{
	printf("hello world!\n");
	return 0;
}
