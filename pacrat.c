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

#include <alpm.h>

#ifndef PACMAN_ROOT
	#define PACMAN_ROOT "/"
#endif

#ifndef PACMAN_DB
	#define PACMAN_DBPATH "/var/lib/pacman"
#endif

static void copy(const char *, const char *);
static void mkpath(const char *, mode_t);
static void archive(const char *path, const char *);
static int is_modified(const char *, const alpm_backup_t *);
static void alpm_do_backup(alpm_pkg_t *);
static void alpm_find_backups(void);
static int parse_options(int, char*[]);

/* runtime configuration */
static struct {
	int opmask;
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

void alpm_do_backup(alpm_pkg_t *pkg) /* {{{ */
{
	const char *pkgname = alpm_pkg_get_name(pkg);
	const alpm_list_t *i;
	char path[PATH_MAX];

	for (i = alpm_pkg_get_backup(pkg); i; i = alpm_list_next(i)) {
		const alpm_backup_t *backup = i->data;

		snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);
		if (is_modified(path, backup) == 0)
			continue;

		printf("%s: %s\n", pkgname, path);
		archive(path, pkgname);
	}
} /* }}} */

void alpm_find_backups(void) /* {{{ */
{
	alpm_db_t *db;
	const alpm_list_t *i;

	db = alpm_get_localdb(pmhandle);
	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i))
		alpm_do_backup(i->data);
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
