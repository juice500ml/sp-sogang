#ifndef __FILECTRL_H__
#define __FILECTRL_H__

#include "queue.h"

#include <stdbool.h>
#include <stdint.h>

#define __OPCODE_FORMAT_SIZE 8

bool is_file (const char *filename);
bool print_file (const char *filename);
void free_oplist (void);
bool init_oplist (const char *filename);
int find_oplist (char *cmd);
void print_oplist (void);
bool assemble_file (const char *filename);

struct op_elem
{
  uint8_t code;
  char format[__OPCODE_FORMAT_SIZE];
  char *opcode;
  struct q_elem elem;
};

struct str_elem
{
  char *line;
  struct q_elem elem;
};

struct sym_elem
{
  char *label;
  int locctr;
  struct q_elem elem;
};

#endif
