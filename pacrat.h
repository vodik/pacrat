#ifndef PACRAT_H
#define PACRAT_H

#define PACMAN_ROOT "/"

typedef struct file_t file_t;
typedef struct backup_t backup_t;

struct file_t {
    char *path;
    char *hash;
};

struct backup_t {
    const char *pkgname;
    file_t system;
    file_t local;
    const char *hash;
};

void backup_free(void *ptr);

alpm_list_t *find_all_modified(alpm_db_t *db);
int copy_modified_to_repo(alpm_list_t *modified, const char *basedir);

#endif

// vim: et:sts=4:sw=4:cino=(0
