#ifndef CONFIG_H
#define CONFIG_H

#include <alpm.h>
#include "pacrat.h"

enum {
	OP_DEBUG = 1000
};

/* runtime configuration */
typedef struct {
	loglevel_t logmask;

	short color;
	int all : 1;

	alpm_list_t *targets;
} options_t;

typedef struct {
	const char *error;
	const char *warn;
	const char *info;
	const char *pkg;
	const char *nc;
} strings_t;

extern options_t cfg;
strings_t colstr;

int parse_options(int, char**);
void strings_init(void);

#endif
