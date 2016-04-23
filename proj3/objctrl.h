#ifndef __OBJCTRL_H__
#define __OBJCTRL_H__

#include "queue.h"
#include "filectrl.h"

#include <stdint.h>
#include <stdbool.h>

uint32_t get_proglen (void);
void set_progaddr (uint32_t addr);
uint32_t get_progaddr (void);
void init_loader (void);
bool add_obj_loader (const char *filename);
bool run_obj_loader (uint8_t *mem);
void free_loader (void);
void print_load_map (void);
bool check_bp (uint32_t addr, uint32_t len);
bool add_bp (uint32_t addr);
void print_bp (void);
void free_bp (void);

struct prog_elem
{
  struct queue *obj_file;
  struct queue *sym_list;
  char *ctrl_name;
  int obj_addr;
  int obj_len;
  int refer[256];
  struct q_elem elem;
};

struct breakpoint
{
  uint32_t *addr;
  int len;
  int cursor;
};

#endif
