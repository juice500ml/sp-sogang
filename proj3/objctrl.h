#ifndef __OBJCTRL_H__
#define __OBJCTRL_H__

#include "queue.h"
#include "filectrl.h"

#include <stdint.h>
#include <stdbool.h>

void set_progaddr (uint32_t addr);
uint32_t get_progaddr (void);
void init_loader (void);
bool add_obj_loader (const char *filename);
void free_loader (void);
void print_load_map (void);

struct prog_elem
{
  struct queue *obj_file;
  struct queue *sym_list;
  char *ctrl_name;
  int obj_addr;
  int obj_len;
  struct q_elem elem;
};

#endif
