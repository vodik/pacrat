#ifndef PACRAT_H
#define PACRAT_H

#include <stdio.h>
#include <alpm.h>
#include "options.h"

#define STREQ(x,y) (strcmp((x),(y)) == 0)

#ifndef PACMAN_ROOT
	#define PACMAN_ROOT "/"
#endif
#ifndef PACMAN_DB
	#define PACMAN_DBPATH "/var/lib/pacman"
#endif

typedef struct file_t {
	char *path;
	char *hash;
} file_t;

typedef struct backup_t {
	const char *pkgname;
	file_t system;
	file_t local;
	const char *hash;
} backup_t;

int cwr_fprintf(FILE *, loglevel_t, const char *, ...) __attribute__((format(printf,3,4)));
int cwr_printf(loglevel_t, const char *, ...) __attribute__((format(printf,2,3)));
int cwr_vfprintf(FILE *, loglevel_t, const char *, va_list) __attribute__((format(printf,3,0)));

void file_init(file_t *, const char *, char *);
void backup_free(void *);

char *get_hash(const char *);
alpm_list_t *alpm_find_backups(alpm_pkg_t *, int);
alpm_list_t *alpm_all_backups(int);

int cmd_status(int, char *[]);
int cmd_pull(int, char *[]);

#endif
