#ifndef __FILECTRL_H__
#define __FILECTRL_H__

#include <stdbool.h>

bool is_file (const char *filename);
bool print_file (const char *filename);
void free_oplist (void);
bool init_oplist (const char *filename);
bool find_oplist (char *cmd);
void print_oplist (void);

#endif
