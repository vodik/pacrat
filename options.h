#ifndef OPTIONS_H
#define OPTIONS_H

#include <alpm.h>

typedef enum loglevel_t {
	LOG_INFO    = 1,
	LOG_ERROR   = (1 << 1),
	LOG_WARN    = (1 << 2),
	LOG_DEBUG   = (1 << 3),
	LOG_VERBOSE = (1 << 4),
	LOG_BRIEF   = (1 << 5)
} loglevel_t;

/* runtime configuration */
typedef struct {
	loglevel_t logmask;

	short color;
	int all : 1;

	alpm_list_t *targets;
} options_t;

enum {
	OP_DEBUG = 1000
};

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
