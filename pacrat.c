#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <locale.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include <alpm.h>

#ifndef PACMAN_ROOT
	#define PACMAN_ROOT "/"
#endif

#ifndef PACMAN_DB
	#define PACMAN_DBPATH "/var/lib/pacman"
#endif

typedef enum __operation_t {
	OP_LIST = 1,
	OP_PULL = (1 << 1),
	OP_PUSH = (1 << 2)
} operation_t;

typedef struct __backup_t {
	const char *pkgname;
	const alpm_backup_t *data;
} backup_t;

static void copy(const char *, const char *);
static void mkpath(const char *, mode_t);
static void archive(const char *path, const char *);
static int is_modified(const char *, const alpm_backup_t *);
static void alpm_do_backup(backup_t *);
static alpm_list_t *alpm_find_backups(void);
static int parse_options(int, char*[]);

/* runtime configuration */
static struct {
	operation_t opmask;
} cfg;

alpm_handle_t *pmhandle;

void copy(const char *src, const char *dest) /* {{{ */
{
	int in  = open(src, O_RDONLY);
	int out = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	char buf[8192];

	ssize_t ret;
	while ((ret = read(in, buf, sizeof(buf))) > 0) {
		if(write(out, buf, (size_t)ret) != ret) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	close(in);
	close(out);
} /* }}} */

void mkpath(const char *path, mode_t mode) /* {{{ */
{
	struct stat st;

	if(stat(path, &st) != 0) {
		if(mkdir(path, mode) != 0) {
			perror("mkdir");
			exit(EXIT_FAILURE);
		}
	}
	else if(!S_ISDIR(st.st_mode)) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
} /* }}} */

void archive(const char *path, const char *pkgname) /* {{{ */
{
	char dest[PATH_MAX];
	char *p = NULL;

	snprintf(dest, PATH_MAX, "%s%s", pkgname, path);
	printf("%s\n", dest);

	for(p = dest + 1; *p; p++) {
		if(*p == '/') {
			*p = '\0';
			mkpath(dest, 0777);
			*p = '/';
		}
	}

	copy(path, dest);
} /* }}} */

int is_modified(const char *path, const alpm_backup_t *backup) /* {{{ */
{
	int ret = 0;

	if(access(path, R_OK) == 0) {
		char *md5sum = alpm_compute_md5sum(path);

		if(!md5sum) {
			perror("alpm_compute_md5sum");
			exit(EXIT_FAILURE);
		}

		ret = strcmp(md5sum, backup->hash) != 0;
		free(md5sum);
	}
	return ret;
} /* }}} */

void alpm_do_backup(const backup_t *b) /* {{{ */
{
	const alpm_backup_t *backup = b->data;
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);
	printf("%s: %s\n", b->pkgname, path);
	archive(path, b->pkgname);
} /* }}} */

alpm_list_t *alpm_find_backups(void) /* {{{ */
{
	alpm_db_t *db;
	const alpm_list_t *i, *j;
	char path[PATH_MAX];
	alpm_list_t *backups = NULL;

	db = alpm_get_localdb(pmhandle);
	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i)) {
		alpm_pkg_t *pkg = i->data;
		const char *pkgname = alpm_pkg_get_name(pkg);

		for (j = alpm_pkg_get_backup(pkg); j; j = alpm_list_next(j)) {
			const alpm_backup_t *backup = j->data;

			snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);
			if (is_modified(path, backup) == 0)
				continue;
			else {
				backup_t *b = malloc(sizeof(backup_t));
				b->pkgname = pkgname;
				b->data = backup;
				printf("backup: %s\n", pkgname);
				backups = alpm_list_add(backups, b);
			}
		}
	}

	return backups;
} /* }}} */

int parse_options(int argc, char *argv[]) /* {{{ */
{
	int opt, option_index = 0;

	static const struct option opts[] = {
		/* operations */
		{"list", no_argument, 0, 'l'},

		/* options */
		{0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "l", opts, &option_index)) != -1) {
		switch(opt) {
			case 'l':
				cfg.opmask |= OP_LIST;
				break;
			default:
				return 1;
		}
	}
	return 0;
} /* }}} */

int main(int argc, char *argv[])
{
	int ret;
	enum _alpm_errno_t err;

	setlocale(LC_ALL, "");

	if ((ret = parse_options(argc, argv)) != 0)
		return ret;

	pmhandle = alpm_initialize(PACMAN_ROOT, PACMAN_DBPATH, &err);
	if(!pmhandle) {
		fprintf(stderr, "failed to initialize alpm: %s\n", alpm_strerror(err));
		goto finish;
	}

	alpm_list_t *backups = alpm_find_backups(), *i;
	for (i = backups; i; i = alpm_list_next(i)) {
		const backup_t *b = i->data;

		printf("pkg: %s\n", b->pkgname);
		alpm_do_backup(b);
	}

finish:
	alpm_release(pmhandle);
	return ret;
}
