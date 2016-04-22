#include "objctrl.h"
#include "filectrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
is_chars_hex (char *l, int len)
{
  int i;
  for (i=0;i<len;++i)
    if (!(l[0]>='0'&&l[0]<='9')
        && !(l[0]>='a'&&l[0]<='f')
        && !(l[0]>='A'&&l[0]<='F'))
      return false;
  return true;
}

static int
parse_chars_hex (char *l, int len, bool sign)
{
  char tmp = l[len];
  int i;
  l[len] = '\0';
  sscanf(l, "%x", &i);
  l[len] = tmp;

  if (sign)
    {
      int s = 1 << (len*4 - 1);
      if (i >= s)
        i -= s * 2;
    }
  return i;
}

static bool
h_syntax_checker (char *l)
{
  return l[0] == 'H'
    && strlen (l) == 19
    && is_chars_hex (&l[7], 12);
}

static bool
t_syntax_checker (char *l)
{
  if (l[0] == 'T'
      && strlen (l) >= 11
      && is_chars_hex (&l[1], 8)
      && strlen (l) % 2 == 1)
    {
      char tmp = l[9];
      int i;

      l[9] = '\0';
      sscanf (&l[7], "%x", &i);
      l[9] = tmp;

      if (strlen (&l[9]) / 2 != parse_chars_hex(&l[7],2,false))
        return false;
      return true;
    }
  return false;
}

static bool
m_syntax_checker (char *l)
{
  return l[0] == 'M'
    && ( strlen (l) >= 11 && strlen (l) <= 16 )
    && is_chars_hex (&l[1], 8)
    && ( l[9] == '+' || l[9] == '-');
}

static bool
e_syntax_checker (char *l)
{
  return l[0] == 'E'
    && (strlen(l) == 1
        || (strlen(l) == 7 && is_chars_hex (&l[1], 6)));
}

static bool
d_syntax_checker (char *l)
{
  if (l[0] == 'D'
      && strlen(l) > 1
      && strlen(l) % 12 == 1)
    {
      int len = strlen(l) / 12;
      int i;
      for (i=0;i<len;++i)
        if (!is_chars_hex (&l[i*12+7], 6))
          return false;
      return true;
    }
  return false;
}

static bool
r_syntax_checker (char *l)
{
  return l[0] == 'R'
    && strlen (l) > 1;
}

static bool
obj_syntax_checker (struct queue *obj_file)
{
  bool is_valid = true;
  struct q_elem *e = q_begin (obj_file);
  for (; e != q_end (obj_file); e = q_next (e))
    {
      char *l = q_entry (e, struct str_elem, elem)->line;
      switch (l[0])
        {
        case '.':
          break;

        case 'H':
          if (!h_syntax_checker (l))
            {
              printf("H RECORD IS BROKE AT [%s]\n", l);
              is_valid = false;
            }
          break;

        case 'T':
          if (!t_syntax_checker (l))
            {
              printf("T RECORD IS BROKE AT [%s]\n", l);
              is_valid = false;
            }
          break;

        case 'M':
          if (!m_syntax_checker (l))
            {
              printf("M RECORD IS BROKE AT [%s]\n", l);
              is_valid = false;
            }
          break;

        case 'E':
          if (!e_syntax_checker (l))
            {
              printf("E RECORD IS BROKE AT [%s]\n", l);
              is_valid = false;
            }
          break;

        case 'D':
          if (!d_syntax_checker (l))
            {
              printf("D RECORD IS BROKE AT [%s]\n", l);
              is_valid = false;
            }
          break;

        case 'R':
          if (!r_syntax_checker (l))
            {
              printf("R RECORD IS BROKE AT [%s]\n", l);
              is_valid = false;
            }
          break;

        default:
          printf ("UNKNOWN RECORD TYPE AT [%s]\n", l);
          is_valid = false;
        }
    }
  return is_valid;
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
