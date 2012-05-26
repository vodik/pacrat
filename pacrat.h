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
static void archive(const char *path, const char *pkgname);
static int is_modified(const char *, const alpm_backup_t *);
static void alpm_do_backup(alpm_pkg_t *pkg);
static void alpm_find_backups(void);
static int parse_options(int, char*[]);
