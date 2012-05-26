#define _GNU_SOURCE
#include <stdlib.h>
#include <getopt.h>
#include <locale.h>
#include <stdio.h>

#include <alpm.h>

static alpm_handle_t *alpm_init(void);
static int parse_options(int, char*[]);

/* runtime configuration */
static struct {
	int opmask;
} cfg;

alpm_handle_t *pmhandle;

alpm_handle_t *alpm_init(void)
{
	return NULL;
}

int parse_options(int argc, char *argv[])
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
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	pmhandle = alpm_init();
	if(!pmhandle) {
		fprintf(stderr, "failed to initialize alpm library\n");
		goto finish;
	}

finish:
	alpm_release(pmhandle);
	return 0;
}
