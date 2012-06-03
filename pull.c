#include "pacrat.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static void copy(const char *, const char *);
static void mkpath(const char *, mode_t);
static void archive(const backup_t *);

void copy(const char *src, const char *dest) /* {{{ */
{
	struct stat st;
	stat(src, &st);

	int in  = open(src, O_RDONLY);
	int out = open(dest, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
	char buf[8192];

	ssize_t ret;
	while ((ret = read(in, buf, sizeof(buf))) > 0) {
		if (write(out, buf, (size_t)ret) != ret) {
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

	if (stat(path, &st) != 0) {
		if (mkdir(path, mode) != 0) {
			perror("mkdir");
			exit(EXIT_FAILURE);
		}
	}
	else if (!S_ISDIR(st.st_mode)) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
} /* }}} */

void archive(const backup_t *backup) /* {{{ */
{
	struct stat st;
	char dest[PATH_MAX];
	char *ptr, *root;
	snprintf(dest, PATH_MAX, "%s%s", backup->pkgname, backup->system.path);

	root = dest + strlen(backup->pkgname);

	for (ptr = dest + 1; *ptr; ptr++) {
		if (*ptr == '/') {
			*ptr = '\0';

			int mode = 0777;
			if (ptr > root) {
				if (stat(root, &st) != 0)
					perror("stat");
				mode = st.st_mode;
			}
			mkpath(dest, mode);

			*ptr = '/';
		}
	}

	copy(backup->system.path, dest);
} /* }}} */

int cmd_pull(int argc, char **argv)
{
	alpm_list_t *backups = alpm_all_backups(cfg.all);
	alpm_list_t *i;

	for (i = backups; i; i = i->next)
		archive(i->data);

	alpm_list_free_inner(backups, backup_free);
	alpm_list_free(backups);

	return 0;
}
