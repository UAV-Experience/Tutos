#ifndef TYPES_DONNEES_H
#define TYPES_DONNEES_H

#include "jansson/jansson.h"
typedef void (*messageFonction)(long, json_t *);

struct function_list_entry {
    const char *key;
    messageFonction ptr ;
};
typedef struct function_list_entry function_list_entry;


#endif // TYPES_DONNEES_H
