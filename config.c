#include "config.h"

#include "pacrat.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

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
			case 'p':
				cfg.opmask |= OP_PULL;
				break;
			case 'l':
				cfg.opmask |= OP_LIST;
				break;
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

#define NOT_EXCL(val) (cfg.opmask & (val) && (cfg.opmask & ~(val)))

	/* check for invalid operation combos */
	if (NOT_EXCL(OP_LIST) || NOT_EXCL(OP_PULL) || NOT_EXCL(OP_PUSH)) {
		fprintf(stderr, "error: invalid operation\n");
		return 2;
	}

	while (optind < argc) {
		if (!alpm_list_find_str(cfg.targets, argv[optind])) {
			cwr_fprintf(stderr, LOG_DEBUG, "adding target: %s\n", argv[optind]);
			cfg.targets = alpm_list_add(cfg.targets, strdup(argv[optind]));
		}
		optind++;
	}

	return 0;
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

