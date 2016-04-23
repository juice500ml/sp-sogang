#include "objctrl.h"
#include "filectrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

static uint32_t progaddr = 0x0;
static uint32_t proglen = 0x0;
static struct queue *obj_list = NULL;

// getter/setter for proglen
static void
set_proglen (uint32_t len)
{
  proglen = len;
}

static uint32_t
get_proglen (void)
{
  return proglen;
}

// check if string is hex
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

// cp str to allocated mem (have to be freed afterwise)
static char *
alloc_str_cpy (char *src, int len)
{
  if (src == NULL)
    {
      puts ("[ERROR][DEBUG] src is null at alloc_str_cpy");
    }
  char *ret = malloc (sizeof(char)*(len+1));
  if (ret == NULL)
    {
      puts ("[LOADER] MEMORY INSUFFICIENT");
      return NULL;
    }
  char tmp = src[len];
  src[len] = '\0';
  sscanf (src, "%s", ret);
  src[len] = tmp;

  len = strlen (ret);
  ret = realloc (ret, len + 1);
  return ret;
}

// parse hex value from string
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

// syntax checkers for records
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

// wrapper for record syntax checkers
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

// find control section length from file (chars queue)
static int
find_cslen (struct queue *obj_file)
{
  if (obj_file == NULL)
    {
      puts ("[ERROR][DEBUG] obj_file IS NULL AT find_cslen");
      return -1;
    }
  struct q_elem *e = q_begin (obj_file);
  int len = -1;
  for (; e != q_end (obj_file); e = q_next (e))
    {
      struct str_elem *se = q_entry (e, struct str_elem, elem);
      if (se->line[0] == 'H')
        {
          // Multiple H record
          if (len >= 0)
            return -2;
          len = parse_chars_hex (&(se->line[13]), 6, false);
        }
    }
  // if -1, no header record
  return len;
}

// find symbol by label from estab
static int
find_sym_list (char *label)
{
  // if -3, no list
  // if -2, multiple label
  // if -1, no addr

  if (obj_list == NULL)
    return -3;

  int addr = -1;
  struct q_elem *e = q_begin (obj_list);
  for (; e != q_end (obj_list); e = q_next (e))
    {
      struct prog_elem *pe =
        q_entry (e, struct prog_elem, elem);
      if (pe->ctrl_name == NULL)
        {
          puts ("[ERROR][DEBUG] NO ctrl_name AT find_sym_list");
          continue;
        }
      if (_SAME_STR(pe->ctrl_name, label))
        {
          if (addr >= 0)
            return -2;
          addr = pe->obj_addr;
        }

      if (pe->sym_list == NULL)
        continue;
      struct q_elem *qe = q_begin (pe->sym_list);
      for (; qe != q_end (pe->sym_list); qe = q_next (qe))
        {
          struct sym_elem *se =
            q_entry (qe, struct sym_elem, elem);
          if (_SAME_STR(se->label, label))
            {
              if (addr >= 0)
                return -2;
              addr = se->locctr;
            }
        }
    }
  return addr;
}

static void
free_sym_list (struct queue *sym_list)
{
  if (sym_list == NULL)
    return;
  while (!q_empty(sym_list))
    {
      struct q_elem *e = q_delete (sym_list);
      struct sym_elem *se = q_entry (e, struct sym_elem, elem);
      if (se->label != NULL)
        free (se->label);
      free (se);
    }
  free (sym_list);
}

static struct queue *
make_sym_list (struct prog_elem *prog)
{
  if (prog == NULL)
    return NULL;

  struct queue *obj_file = prog->obj_file;
  struct q_elem *e = q_begin (obj_file);
  struct queue *q = malloc (sizeof (struct queue));
  if (q == NULL)
    {
      puts ("[LOADER] MEMORY INSUFFICIENT");
      return NULL;
    }
  q_init (q);

  for (; e != q_end (obj_file); e = q_next (e))
    {
      struct str_elem *se = q_entry (e, struct str_elem, elem);
      if (se->line[0] == 'D')
        {
          // make entry for each elem in D recrod
          int len = strlen (se->line) / 12;
          int i;
          for (i = 0; i < len; ++i)
            {
              char *label =
                alloc_str_cpy(&(se->line[1+i*12]),6);
              if (label == NULL)
                {
                  puts ("[LOADER] MEMORY INSUFFICIENT");
                  free (label);
                  free_sym_list (q);
                  return NULL;
                }
              int ret = find_sym_list (label);
              // label already there
              if (ret > 0)
                {
                  printf("[LOADER] MULTIPLE LABEL %s\n", label);
                  free (label);
                  free_sym_list (q);
                  return NULL;
                }
              struct sym_elem *syme =
                malloc (sizeof (struct sym_elem));
              if (syme == NULL)
                {
                  puts ("[LOADER] MEMORY INSUFFICIENT");
                  free (label);
                  free_sym_list (q);
                  return NULL;
                }

              // insert into estab
              syme->label = label;
              // obj_addr + hex value from D record
              syme->locctr =
                parse_chars_hex (se->line+7+i*12, 6, false)
                + prog->obj_addr;
              q_insert (q, &syme->elem);
            }
        }
    }
  return q;
}

// find cs name from H record
static char *
find_ctr_name (struct queue *obj_file)
{
  struct q_elem *e = q_begin (obj_file);
  char *name = NULL;
  for (; e != q_end (obj_file); e = q_next (e))
    {
      struct str_elem *se = q_entry (e, struct str_elem, elem);
      if (se->line[0] == 'H')
        {
          // Multiple H record
          if (name != NULL)
            {
              free (name);
              return NULL;
            }
          name = alloc_str_cpy (&(se->line[1]), 6);
          if (name == NULL)
            {
              puts ("[LOADER] MEMORY INSUFFICIENT");
              return NULL;
            }
        }
    }
  // if NULL, no header record
  return name;
}

void
init_loader (void)
{
  free_loader ();
  proglen = 0x0;
  obj_list = malloc (sizeof(struct queue));
  q_init (obj_list);
}

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

      free_file (pe->obj_file);

      free_sym_list (pe->sym_list);

      if (pe->ctrl_name != NULL)
        free (pe->ctrl_name);
      free (pe);
    }
  obj_list = NULL;
}

// getter/setter for progaddr
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

// add obj for loading (pass1)
bool
add_obj_loader (const char *filename)
{
  if (!is_file (filename))
    return false;

  struct prog_elem *pe = malloc (sizeof(struct prog_elem));
  if (pe == NULL)
    {
      puts ("[LOADER] MEMORY INSUFFICIENT");
      return false;
    }
  pe->obj_file = NULL;
  pe->sym_list = NULL;
  pe->ctrl_name = NULL;

  pe->obj_file = save_file (filename);
  if (!obj_syntax_checker(pe->obj_file))
    {
      printf ("[%s] OBJ FILE SYNTAX WRONG\n", filename);
      free_file (pe->obj_file);
      free (pe);
      return false;
    }

  pe->ctrl_name = find_ctr_name (pe->obj_file);
  if (pe->ctrl_name == NULL)
    {
      printf ("[%s] NO CONTROL NAME\n", filename);
      free (pe);
      return false;
    }

  pe->obj_addr = get_progaddr () + get_proglen ();

  pe->obj_len = find_cslen (pe->obj_file);
  if (pe->obj_len < 0)
    {
      if (pe->obj_len == -1)
        printf ("[%s] NO HEADER AT OBJ FILE\n", filename);
      else if (pe->obj_len == -2)
        printf ("[%s] MULTIPLE HEADER AT OBJ FILE\n", filename);
      free_file (pe->obj_file);
      free (pe);
      return false;
    }
  set_proglen (get_proglen() + pe->obj_len);

  q_insert (obj_list, &pe->elem);
  pe->sym_list = make_sym_list (pe);
  if (pe->sym_list == NULL)
    {
      printf ("[%s] SYMBOL LIST GENERATION FAILED\n", filename);
      free_file (pe->obj_file);
      free (pe);
      return false;
    }

  return true;
}

// print load map of estab
void
print_load_map (void)
{
  int i = 0;
  if (obj_list == NULL)
    {
      puts ("[ERROR][DEBUG] NO obj_list AT print_load_map");
      return;
    }
  puts ("control\t\tsymbol\t\taddress\t\tlength");
  puts ("section\t\tname");

  for (i=0;i<60;++i) putchar('-');
  puts ("");

  // printing values
  struct q_elem *e = q_begin (obj_list);
  for (; e != q_end (obj_list); e = q_next (e))
    {
      struct prog_elem *pe = q_entry(e, struct prog_elem, elem);
      printf("%s\t\t      \t\t%04X\t\t%04X\n",
             pe->ctrl_name, pe->obj_addr, pe->obj_len);
      struct q_elem *qe = q_begin (pe->sym_list);
      for (; qe != q_end (pe->sym_list); qe = q_next (qe))
        {
          struct sym_elem *se =
            q_entry(qe, struct sym_elem, elem);
          printf("      \t\t%s\t\t%04X\n", se->label, se->locctr);
        }
    }

  for (i=0;i<60;++i) putchar('-');
  puts ("");
  printf ("      \t\t      \t\ttotal length\t%04X\n",
          get_proglen ());
}
