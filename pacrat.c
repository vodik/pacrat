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

/* macros {{{ */
#define STREQ(x,y)            (strcmp((x),(y)) == 0)

#ifndef PACMAN_ROOT
	#define PACMAN_ROOT "/"
#endif
#ifndef PACMAN_DB
	#define PACMAN_DBPATH "/var/lib/pacman"
#endif

#define NC          "\033[0m"
#define BOLD        "\033[1m"

#define BLACK       "\033[0;30m"
#define RED         "\033[0;31m"
#define GREEN       "\033[0;32m"
#define YELLOW      "\033[0;33m"
#define BLUE        "\033[0;34m"
#define MAGENTA     "\033[0;35m"
#define CYAN        "\033[0;36m"
#define WHITE       "\033[0;37m"
#define BOLDBLACK   "\033[1;30m"
#define BOLDRED     "\033[1;31m"
#define BOLDGREEN   "\033[1;32m"
#define BOLDYELLOW  "\033[1;33m"
#define BOLDBLUE    "\033[1;34m"
#define BOLDMAGENTA "\033[1;35m"
#define BOLDCYAN    "\033[1;36m"
#define BOLDWHITE   "\033[1;37m"
/* }}} */

typedef enum __loglevel_t {
	LOG_INFO    = 1,
	LOG_ERROR   = (1 << 1),
	LOG_WARN    = (1 << 2),
	LOG_DEBUG   = (1 << 3),
	LOG_VERBOSE = (1 << 4),
	LOG_BRIEF   = (1 << 5)
} loglevel_t;

typedef enum __operation_t {
	OP_LIST = 1,
	OP_PULL = (1 << 1),
	OP_PUSH = (1 << 2)
} operation_t;

enum {
	OP_DEBUG = 1000
};

typedef struct __backup_t {
	const char *pkgname;
	const char *path;
	const char *hash;
} backup_t;

typedef struct __strings_t {
	const char *error;
	const char *warn;
	const char *info;
	const char *pkg;
	const char *nc;
} strings_t;

static int cwr_fprintf(FILE *, loglevel_t, const char *, ...) __attribute__((format(printf,3,4)));
static int cwr_printf(loglevel_t, const char *, ...) __attribute__((format(printf,2,3)));
static int cwr_vfprintf(FILE *, loglevel_t, const char *, va_list) __attribute__((format(printf,3,0)));
static void copy(const char *, const char *);
static void mkpath(const char *, mode_t);
static void archive(const backup_t *);
static int is_modified(const char *, const alpm_backup_t *);
static alpm_list_t *alpm_find_backups(alpm_pkg_t *, int);
static alpm_list_t *alpm_all_backups(int);
static alpm_list_t *stored_backups(alpm_pkg_t *pkg, char *dir);
static int parse_options(int, char*[]);
static int strings_init(void);
static void print_backup(backup_t *);
static void usage(void);
static void version(void);

/* runtime configuration */
static struct {
	operation_t opmask;
	loglevel_t logmask;

	short color;
	int all : 1;
} cfg;

alpm_handle_t *pmhandle;
strings_t *colstr;

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
			prefix = colstr->info;
			break;
		case LOG_ERROR:
			prefix = colstr->error;
			break;
		case LOG_WARN:
			prefix = colstr->warn;
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

void copy(const char *src, const char *dest) /* {{{ */
{
	int in  = open(src, O_RDONLY);
	int out = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0666);
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
	char dest[PATH_MAX];
	char *p = NULL;
	snprintf(dest, PATH_MAX, "%s%s", backup->pkgname, backup->path);

	for (p = dest + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			mkpath(dest, 0777);
			*p = '/';
		}
	}

	copy(backup->path, dest);
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

alpm_list_t *alpm_find_backups(alpm_pkg_t *pkg, int everything) /* {{{ */
{
	const alpm_list_t *i;
	char path[PATH_MAX];
	alpm_list_t *backups = NULL;

	const char *pkgname = alpm_pkg_get_name(pkg);

	for (i = alpm_pkg_get_backup(pkg); i; i = alpm_list_next(i)) {
		const alpm_backup_t *backup = i->data;

		snprintf(path, PATH_MAX, "%s%s", PACMAN_ROOT, backup->name);
		if (!everything && is_modified(path, backup) == 0)
			continue;
		else {
			backup_t *b = malloc(sizeof(backup_t));
			b->pkgname = pkgname;
			b->path = strdup(path);
			b->hash = backup->hash;

			cwr_fprintf(stderr, LOG_DEBUG, "found backup: %s\n", path);
			backups = alpm_list_add(backups, b);
		}
	}

	return backups;
} /* }}} */

alpm_list_t *alpm_all_backups(int everything) /* {{{ */
{
	alpm_db_t *db;
	const alpm_list_t *i;
	alpm_list_t *backups = NULL;

	db = alpm_get_localdb(pmhandle);
	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i)) {
		alpm_list_t *pkg_backups = alpm_find_backups(i->data, everything);
		backups = alpm_list_join(backups, pkg_backups);
	}

	return backups;
} /* }}} */

alpm_list_t *stored_backups(alpm_pkg_t *pkg, char *dir) /* {{{ */
{
	char fileloc[PATH_MAX];
	struct stat buf;
	size_t status;
	alpm_list_t *files = NULL, *i;
	for (i = alpm_find_backups(pkg,1); i; alpm_list_next(i)) {
		snprintf(fileloc, PATH_MAX, "%s/%s/%s", dir, alpm_pkg_get_name(pkg), (char *)i);
		status = stat(fileloc, &buf);
		if (status == 0 && S_ISREG (buf.st_mode)){
			files = alpm_list_add(files, fileloc);
			cwr_fprintf(stderr, LOG_DEBUG, "adding file: %s\n", fileloc);
			printf("%s\n", fileloc);
		}
	}
	return files;
} /* }}} */

int parse_options(int argc, char *argv[]) /* {{{ */
{
	int opt, option_index = 0;

	static const struct option opts[] = {
		/* operations */
		{"list",    no_argument,       0, 'l'},

		/* options */
		{"all",     no_argument,       0, 'a'},
		{"color",   optional_argument, 0, 'c'},
		{"debug",   no_argument,       0, OP_DEBUG},
		{"help",    no_argument,       0, 'h'},
		{"verbose", no_argument,       0, 'v'},
		{"version", no_argument,       0, 'V'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "lac:hvV", opts, &option_index)) != -1) {
		switch(opt) {
			case 'a':
				cfg.all |= 1;
				break;
			case 'l':
				cfg.opmask |= OP_LIST;
				break;
			case 'c':
				if (!optarg || STREQ(optarg, "auto")) {
					if (isatty(fileno(stdout))) {
						cfg.color = 1;
					} else {
						cfg.color = 0;
					}
				} else if (STREQ(optarg, "always")) {
					cfg.color = 1;
				} else if (STREQ(optarg, "never")) {
					cfg.color = 0;
				} else {
					fprintf(stderr, "invalid argument to --color\n");
					return 1;
				}
				break;
			case 'h':
				usage();
				return 1;
			case 'v':
				cfg.logmask |= LOG_VERBOSE;
				break;
			case 'V':
				version();
				return 2;
			case OP_DEBUG:
				cfg.logmask |= LOG_DEBUG;
				break;
			default:
				return 1;
		}
	}
	return 0;
} /* }}} */

int strings_init(void) /* {{{ */
{
	colstr = malloc(sizeof(strings_t));
	if (!colstr)
		return 1;

	if (cfg.color > 0) {
		colstr->error = BOLDRED "::" NC;
		colstr->warn  = BOLDYELLOW "::" NC;
		colstr->info  = BOLDBLUE "::" NC;
		colstr->pkg   = BOLD;
		colstr->nc    = NC;
	} else {
		colstr->error = "error:";
		colstr->warn  = "warning:";
		colstr->info  = "::";
		colstr->pkg   = "";
		colstr->nc    = "";
	}

	return 0;
} /* }}} */

void print_backup(backup_t *b) /* {{{ */
{
	printf("%s %s%s%s %s\n", colstr->info, colstr->pkg, b->pkgname, colstr->nc, b->path);
} /* }}} */

void usage(void) /* {{{ */
{
	fprintf(stderr, "pacrat %s\n"
			"Usage: pacrat <operations> [options]...\n\n", PACRAT_VERSION);
	fprintf(stderr,
			" Operations:\n"
			"  -u, --update            check for updates against AUR -- can be combined "
			"with the -d flag\n\n");
	fprintf(stderr, " General options:\n"
			"  -h, --help              display this help and exit\n"
			"  -V, --version           display version\n\n");
	fprintf(stderr, " Output options:\n"
			"  -c, --color[=WHEN]      use colored output. WHEN is `never', `always', or `auto'\n"
			"      --debug             show debug output\n"
			"  -v, --verbose           output more\n\n");
} /* }}} */

void version(void) /* {{{ */
{
	printf("\n " PACRAT_VERSION "\n");
	printf("     \\   (\\,/)\n"
		   "      \\  oo   '''//,        _\n"
		   "       ,/_;~,       \\,     / '\n"
		   "       \"'   \\    (    \\    !\n"
		   "             ',|  \\    |__.'\n"
		   "             '~  '~----''\n"
		   "\n"
		   "             Pacrat....\n\n");
} /* }}} */

int main(int argc, char *argv[])
{
	int ret;
	enum _alpm_errno_t err;

	setlocale(LC_ALL, "");

	if ((ret = parse_options(argc, argv)) != 0)
		return ret;

	cwr_printf(LOG_DEBUG, "initializing alpm\n");
	pmhandle = alpm_initialize(PACMAN_ROOT, PACMAN_DBPATH, &err);
	if (!pmhandle) {
		cwr_fprintf(stderr, LOG_ERROR, "failed to initialize alpm library\n");
		goto finish;
	}

	if((ret = strings_init()) != 0) {
		return ret;
	}

	if (cfg.opmask & OP_LIST) {
		alpm_list_t *backups = alpm_all_backups(cfg.all), *i;
		for (i = backups; i; i = alpm_list_next(i))
			print_backup(i->data);
	} else {
		alpm_list_t *backups = alpm_all_backups(cfg.all), *i;
		for (i = backups; i; i = alpm_list_next(i))
			archive(i->data);
	}

finish:
	alpm_release(pmhandle);
	return ret;
}
/* vim: set ts=4 sw=4 noet: */
