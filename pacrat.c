#define _GNU_SOURCE
#include <stdlib.h>
#include <getopt.h>
#include <locale.h>
#include <stdio.h>

#include <alpm.h>

#ifndef PACMAN_ROOT
	#define PACMAN_ROOT "/"
#endif

#ifndef PACMAN_DB
	#define PACMAN_DBPATH "/var/lib/pacman"
#endif

static void alpm_find_backups(void);
static int parse_options(int, char*[]);

/* runtime configuration */
static struct {
	int opmask;
} cfg;

alpm_handle_t *pmhandle;

void alpm_find_backups(void) /* {{{ */
{
	alpm_db_t *db;
	const alpm_list_t *i, *j;

	db = alpm_get_localdb(pmhandle);
	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i)) {
		const char *pkgname = alpm_pkg_get_name(i->data);
		for (j = alpm_pkg_get_backup(i->data); j; j = alpm_list_next(j)) {/* {{{ *//* }}} */
			const alpm_backup_t *backup = j->data;
			printf("%s: %s\n", pkgname, backup->name);
		}
	}
} /* }}} */

int parse_options(int argc, char *argv[]) /* {{{ */
{
	int opt, option_index = 0;

	static const struct option opts[] = {
		/* operations */

		/* options */
		{0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "", opts, &option_index)) != -1) {
		switch(opt) {
		}
	}
	return 0;
} /* }}} */

int main(int argc, char *argv[])
{
	int ret;
	enum _alpm_errno_t err;

	setlocale(LC_ALL, "");

	ret = 0;
	/* ret = parse_options(argc, argv); */

	pmhandle = alpm_initialize(PACMAN_ROOT, PACMAN_DBPATH, &err);
	if(!pmhandle) {
		fprintf(stderr, "failed to initialize alpm: %s\n", alpm_strerror(err));
		goto finish;
	}

	alpm_find_backups();

finish:
	alpm_release(pmhandle);
	return ret;
}
