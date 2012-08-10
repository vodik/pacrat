#ifndef ACTIONS_H
#define ACTIONS_H

#include <stdio.h>
#include <alpm.h>

typedef struct action_t action_t;

struct action_t {
    void (*parsearg)(char);
    int (*action)(alpm_list_t *);
    void (*usage)(FILE *);
};

extern const action_t list_action;
extern const action_t pull_action;

#endif

// vim: et:sts=4:sw=4:cino=(0
