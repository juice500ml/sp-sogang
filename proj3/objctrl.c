#include "objctrl.h"
#include "filectrl.h"

#include <stdio.h>
#include <stdlib.h>

static uint32_t progaddr = 0x0;
static struct queue *obj_list = NULL;

void
free_loader (void)
{
  if (obj_list == NULL)
    return;
  while (!q_empty (obj_list))
    {
      struct q_elem *e = q_delete (obj_list);
      struct prog_elem *pe
        = q_entry (e, struct prog_elem, elem);

      struct queue *obj_file = pe->obj_file;
      while (obj_file != NULL && !q_empty (obj_file))
        {
          struct q_elem *e = q_delete (obj_file);
          struct str_elem *se
            = q_entry (e, struct str_elem, elem);
          if (se->line != NULL)
            free (se->line);
          free (se);
        }
      if (obj_file != NULL)
        free (obj_file);

      struct queue *sym_list = pe->sym_list;
      while (sym_list != NULL && !q_empty (sym_list))
        {
          struct q_elem *e = q_delete (sym_list);
          struct sym_elem *se
            = q_entry (e, struct sym_elem, elem);
          if (se->label != NULL)
            free (se->label);
          free (se);
        }
      if (sym_list != NULL)
        free (sym_list);

      if (pe->ctrl_name != NULL)
        free (pe->ctrl_name);
      free (pe);
    }
  obj_list = NULL;
}

void
set_progaddr (uint32_t addr)
{
  progaddr = addr;
}

uint32_t
get_progaddr (void)
{
  return progaddr;
}

static bool
obj_syntax_checker (struct queue *obj_file)
{
  struct q_elem *e = q_begin (obj_file);
  for (; e != q_end (obj_file); e = q_next (e))
    {
      char *l = q_entry (e, struct str_elem, elem)->line;
      puts (l);
    }
  return false;
}

void
init_loader (void)
{
  free_loader ();
  obj_list = malloc (sizeof(struct queue));
  q_init (obj_list);
}

bool
add_obj_loader (const char *filename)
{
  if (!is_file (filename))
    return false;
  struct prog_elem *pe = malloc (sizeof(struct prog_elem));
  pe->obj_file = save_file (filename);
  if (!obj_syntax_checker(pe->obj_file))
    {
      free_file (pe->obj_file);
      free (pe);
      return false;
    }
  pe->sym_list = NULL;

  return true;
}
