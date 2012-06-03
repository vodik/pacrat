#include "options.h"

#include "pacrat.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

/* macros {{{ */
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

options_t cfg = {
	.logmask = LOG_ERROR | LOG_WARN | LOG_INFO,
	.color   = 1
};

static void usage(void);
static void version(void);

int parse_options(int argc, char **argv) /* {{{ */
{
	int opt, option_index = 0;

	static const struct option opts[] = {
		/* operations */
		{"pull",    no_argument,       0, 'p'},
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

	while ((opt = getopt_long(argc, argv, "plac:hvV", opts, &option_index)) != -1) {
		switch(opt) {
			/* case 'p': */
			/* 	cfg.opmask |= OP_PULL; */
			/* 	break; */
			/* case 'l': */
			/* 	cfg.opmask |= OP_LIST; */
			/* 	break; */
			case 'a':
				cfg.all |= 1;
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

/* #define NOT_EXCL(val) (cfg.opmask & (val) && (cfg.opmask & ~(val))) */

	/* check for invalid operation combos */
	/* if (NOT_EXCL(OP_LIST) || NOT_EXCL(OP_PULL) || NOT_EXCL(OP_PUSH)) { */
	/* 	fprintf(stderr, "error: invalid operation\n"); */
	/* 	return 2; */
	/* } */

	while (optind < argc) {
		if (!alpm_list_find_str(cfg.targets, argv[optind])) {
			cwr_fprintf(stderr, LOG_DEBUG, "adding target: %s\n", argv[optind]);
			cfg.targets = alpm_list_add(cfg.targets, strdup(argv[optind]));
		}
		optind++;
	}

	return 0;
} /* }}} */

void strings_init(void) /* {{{ */
{
	if (cfg.color > 0) {
		colstr.error = BOLDRED "::" NC;
		colstr.warn  = BOLDYELLOW "::" NC;
		colstr.info  = BOLDBLUE "::" NC;
		colstr.pkg   = BOLD;
		colstr.nc    = NC;
	} else {
		colstr.error = "error:";
		colstr.warn  = "warning:";
		colstr.info  = "::";
		colstr.pkg   = "";
		colstr.nc    = "";
	}
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

