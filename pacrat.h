#ifndef PACRAT_H
#define PACRAT_H

#define PACMAN_ROOT "/"

#include <stdio.h>
#include <alpm.h>

typedef struct file_t file_t;
typedef struct backup_t backup_t;

struct action_t {
    const char *name;
    void (*parsearg)(char);
    void (*usage)(FILE *);
    int (*cmd)(alpm_list_t *);
};

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

extern const struct action_t list_action;

#endif

// vim: et:sts=4:sw=4:cino=(0
