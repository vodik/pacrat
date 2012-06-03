#include "pacrat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <locale.h>
#include <sys/stat.h>

#include <alpm.h>
#include "git.h"

typedef int (*cmdhandler)(int argc, char *argv[]);

typedef struct command_t {
	const char *command;
	cmdhandler handler;
	const char *usage;
} command_t;

static command_t pacrat_cmds[] = {
	/* command, handler,    help msg */
	{"status",  cmd_status, "TODO"},
	{"pull",    cmd_pull,   "TODO"},
	{NULL}
};

typedef enum pacfiles_t {
	CONF_PACNEW  = 1,
	CONF_PACSAVE = (1 << 1),
	CONF_PACORIG = (1 << 2)
} pacfiles_t;

alpm_handle_t *pmhandle;

static pacfiles_t check_pacfiles(const char *);
static const command_t *find(const char *);
static int run(const command_t *, int, char *[]);

int cwr_fprintf(FILE *stream, loglevel_t level, const char *format, ...) /* {{{ */
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = cwr_vfprintf(stream, level, format, args);
	va_end(args);

	return ret;
} /* }}} */

int cwr_printf(loglevel_t level, const char *format, ...) /* {{{ */
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = cwr_vfprintf(stdout, level, format, args);
	va_end(args);

	return ret;
} /* }}} */

int cwr_vfprintf(FILE *stream, loglevel_t level, const char *format, va_list args) /* {{{ */
{
	const char *prefix;
	char bufout[128];

	if (!(cfg.logmask & level))
		return 0;

	switch (level) {
		case LOG_VERBOSE:
		case LOG_INFO:
			prefix = colstr.info;
			break;
		case LOG_ERROR:
			prefix = colstr.error;
			break;
		case LOG_WARN:
			prefix = colstr.warn;
			break;
		case LOG_DEBUG:
			prefix = "debug:";
			break;
		default:
			prefix = "";
			break;
	}

	/* f.l.w.: 128 should be big enough... */
	snprintf(bufout, 128, "%s %s", prefix, format);

	return vfprintf(stream, bufout, args);
} /* }}} */

void file_init(file_t *file, const char *path, char *hash) /* {{{ */
{
	file->hash = hash ? hash : get_hash(path);
	file->path = strdup(path);
} /* }}} */

void backup_free(void *ptr) /* {{{ */
{
	backup_t *backup = ptr;
	free(backup->system.path);
	free(backup->system.hash);
	free(backup->local.path);
	free(backup->local.hash);
	free(backup);
} /* }}} */

char *get_hash(const char *path) /* {{{ */
{
	char *hash = alpm_compute_md5sum(path);
	if(!hash) {
		cwr_fprintf(stderr, LOG_ERROR, "failed to compute hash for %s\n", path);
		exit(EXIT_FAILURE);
	}
	return hash;
} /* }}} */

pacfiles_t check_pacfiles(const char *file) /* {{{ */
{
	char path[PATH_MAX];
	int ret = 0;

	snprintf(path, PATH_MAX, "%s.pacnew", file);
	if (access(path, R_OK) == 0)
		ret |= CONF_PACNEW;

	snprintf(path, PATH_MAX, "%s.pacsave", file);
	if (access(path, R_OK) == 0)
		ret |= CONF_PACSAVE;

	snprintf(path, PATH_MAX, "%s.pacorig", file);
	if (access(path, R_OK) == 0)
		ret |= CONF_PACORIG;

	return ret;
} /* }}} */

alpm_list_t *alpm_find_backups(alpm_pkg_t *pkg, int everything) /* {{{ */
{
	alpm_list_t *backups = NULL;
	const alpm_list_t *i;
	struct stat st;
	char path[PATH_MAX];

	const char *pkgname = alpm_pkg_get_name(pkg);

	for (i = alpm_pkg_get_backup(pkg); i; i = i->next) {
		const alpm_backup_t *backup = i->data;

		snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);

		/* check if we can access the file */
		if (access(path, R_OK) != 0) {
			cwr_fprintf(stderr, LOG_WARN, "can't access %s\n", path);
			continue;
		}

		/* check if there is a pacnew/pacsave/pacorig file */
		pacfiles_t pacfiles = check_pacfiles(path);
		if (pacfiles & CONF_PACNEW)
			cwr_fprintf(stderr, LOG_WARN, "pacnew file detected %s\n", path);
		if (pacfiles & CONF_PACSAVE)
			cwr_fprintf(stderr, LOG_WARN, "pacsave file detected %s\n", path);
		if (pacfiles & CONF_PACORIG)
			cwr_fprintf(stderr, LOG_WARN, "pacorig file detected %s\n", path);

		/* filter unmodified files */
		char *hash = get_hash(path);
		if (!everything && STREQ(backup->hash, hash)) {
			free(hash);
			continue;
		}

		cwr_fprintf(stderr, LOG_DEBUG, "found backup: %s\n", path);

		/* mark the file to be operated on then */
		backup_t *b = malloc(sizeof(backup_t));
		memset(b, 0, sizeof(backup_t));

		b->pkgname = pkgname;
		b->hash = backup->hash;
		file_init(&b->system, path, hash);

		/* look for a local copy */
		snprintf(path, PATH_MAX, "%s/%s", pkgname, backup->name);

		size_t status = stat(path, &st);
		if (status == 0 && S_ISREG (st.st_mode)) {
			cwr_fprintf(stderr, LOG_DEBUG, "found local copy: %s\n", path);
			file_init(&b->local, path, NULL);
		}

		backups = alpm_list_add(backups, b);
	}

	return backups;
} /* }}} */

alpm_list_t *alpm_all_backups(int everything) /* {{{ */
{
	alpm_list_t *backups = NULL;
	const alpm_list_t *i;

	alpm_db_t *db = alpm_get_localdb(pmhandle);
	alpm_list_t *targets = cfg.targets ? alpm_db_search(db, cfg.targets) : alpm_db_get_pkgcache(db);

	for (i = targets; i; i = i->next) {
		alpm_list_t *pkg_backups = alpm_find_backups(i->data, everything);
		backups = alpm_list_join(backups, pkg_backups);
	}

	return backups;
} /* }}} */

const command_t *find(const char *name) /* {{{ */
{
	for (unsigned i = 0; pacrat_cmds[i].command != NULL; ++i) {
		if (STREQ(name, pacrat_cmds[i].command))
			return &pacrat_cmds[i];
	}
	return NULL;
} /* }}} */

int run(const command_t *cmd, int argc, char *argv[]) /* {{{ */
{
	int ret = cmd->handler(argc, argv);
	return (ret >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
} /* }}} */

int main(int argc, char *argv[])
{
	int ret;
	enum _alpm_errno_t err;

	setlocale(LC_ALL, "");
	strings_init();

	if (argc == 1) {
		cwr_fprintf(stderr, LOG_ERROR, "not enough arguments\n");
		exit(EXIT_FAILURE);
	}

	const command_t *cmd = find(argv[1]);
	if (!cmd) {
		cwr_fprintf(stderr, LOG_ERROR, "command %s not understood\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	/* if ((ret = parse_options(argc, argv)) != 0) */
	/* 	return ret; */

	cwr_fprintf(stderr, LOG_DEBUG, "initializing alpm\n");
	pmhandle = alpm_initialize(PACMAN_ROOT, PACMAN_DBPATH, &err);
	if (!pmhandle) {
		cwr_fprintf(stderr, LOG_ERROR, "failed to initialize alpm library\n");
		goto finish;
	}

	if ((ret = repo_open()) != 0) {
		cwr_fprintf(stderr, LOG_ERROR, "repo_open failed\n");
		goto finish;
	}

	ret = run(cmd, --argc, ++argv);

finish:
	cwr_fprintf(stderr, LOG_DEBUG, "releasing alpm\n");
	alpm_release(pmhandle);
	return ret;
}
/* vim: set ts=4 sw=4 noet: */
