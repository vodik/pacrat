#include "git.h"

#include <git2.h>
#include "pacrat.h"

git_repository *repo = NULL;

void repo_open(void)
{
	char path[GIT_PATH_MAX];

	git_repository_discover(path, sizeof(path), ".", 0, "/");
	cwr_fprintf(stderr, LOG_DEBUG, "discovered repo: %s\n", path);

	git_repository_open(&repo, path);
}
