#include "git.h"

#include <git2.h>
#include "pacrat.h"

git_repository *repo = NULL;

int repo_open(void)
{
	char path[GIT_PATH_MAX];
	int ret;

	cwr_fprintf(stderr, LOG_DEBUG, "checking for git repo\n");
	if ((ret = git_repository_discover(path, sizeof(path), ".", 0, "/")) != 0) {
		switch (ret) {
			case GIT_ENOTFOUND:
				cwr_fprintf(stderr, LOG_ERROR, "git repo not found\n");
				break;
			default:
				cwr_fprintf(stderr, LOG_ERROR, "giterror: %d\n", ret);
				break;
		}
		return ret;
	}

	cwr_fprintf(stderr, LOG_DEBUG, "discovered repo: %s\n", path);
	ret = git_repository_open(&repo, path);
	return ret;
}
