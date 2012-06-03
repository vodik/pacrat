#ifndef CONFIG_H
#define CONFIG_H

#include <alpm.h>
#include "pacrat.h"

enum {
	OP_DEBUG = 1000
};

/* runtime configuration */
static struct {
	loglevel_t logmask;

	short color;
	int all : 1;

	alpm_list_t *targets;
} cfg;

int parse_options(int, char**);

#endif
