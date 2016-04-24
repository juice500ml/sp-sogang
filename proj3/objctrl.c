#include "objctrl.h"
#include "filectrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

#define __REGISTER_TABLE_SIZE 10
#define __REGISTER_SIZE (1 << 24)
#define __BOOT_ADDRESS ((1 << 20) - 1)
#define __INVALID (1 << 21)


static uint32_t progaddr = 0x0;
static uint32_t startaddr = 0x0;
static uint32_t proglen = 0x0;
static struct queue *obj_list = NULL;
static struct breakpoint *bp = NULL;
static uint32_t *reg = NULL;

// getter/setter for startaddr
static void
set_startaddr (uint32_t addr)
{
  startaddr = addr;
}

static uint32_t
get_startaddr (void)
{
  return startaddr;
}

// getter/setter for proglen
static void
set_proglen (uint32_t len)
{
  proglen = len;
}

uint32_t
get_proglen (void)
{
  return proglen;
}

// getter/setter for progaddr
void
set_progaddr (uint32_t addr)
{
  progaddr = addr;
  set_startaddr (addr);
}

uint32_t
get_progaddr (void)
{
  return progaddr;
}

// validate as validate address inside program
// if not validated, it is seg fault
static bool
validate_addr (uint32_t addr)
{
  return addr >= get_progaddr ()
    && addr <= get_progaddr () + get_proglen ();
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
  int i = 0;
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
  int len = strlen(l);
  int i;

  for (i = 1; i < len; i += 8)
    {
      if (!is_chars_hex (&l[i], 2))
        return false;
      if (parse_chars_hex (&l[i], 2, false) < 0)
        return false;
      char *tmp = alloc_str_cpy (&l[i+2], 6);
      int tmplen = strlen (tmp);
      free (tmp);
      if (tmplen <= 0)
        return false;
    }
  return l[0] == 'R';
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

// freeing symbol list (with itself)
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
  struct queue *q = malloc (sizeof (struct queue));
  int vaddr = 0;
  if (q == NULL)
    {
      puts ("[LOADER] MEMORY INSUFFICIENT");
      return NULL;
    }
  q_init (q);

  struct q_elem *e = q_begin (obj_file);
  for (; e != q_end (obj_file); e = q_next (e))
    {
      struct str_elem *se = q_entry (e, struct str_elem, elem);
      if (se->line[0] == 'H')
        vaddr = parse_chars_hex (se->line+7, 6, false);
    }

  e = q_begin (obj_file);
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
                + prog->obj_addr
                - vaddr;
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

// get val from mem
static uint64_t
get_mem (uint8_t *mem, uint8_t half_bytes)
{
  uint8_t len = half_bytes;
  uint64_t ori = 0;
  int i = 0;

  if (len > 128)
    puts ("[ERROR][DEBUG] get_mem half_bytes TOO BIG");

  for (i = 0; i < (len+1)/2; ++i)
    {
      ori <<= 8;
      if (i==0 && len%2==1)
        ori += mem[i] % (1<<4);
      else
        ori += mem[i];
    }
  return ori;
}

// set val for mem
static void
set_mem (uint8_t *mem, uint8_t half_bytes, uint64_t val)
{
  uint8_t len = half_bytes;
  int i = 0;

  if (len > 128)
    puts ("[ERROR][DEBUG] get_mem half_bytes TOO BIG");

  if (len == 0)
    return;

  for (i = (len-1)/2; i >= 0; --i)
    {
      if (i==0 && len%2==1)
        mem[i] = (mem[i]/(1<<4))*(1<<4) + val%(1<<4);
      else
        mem[i] = val%(1<<8);
      val >>= 8;
    }
}

// add val to mem pointing (len half bytes)
static void
add_mem (uint8_t *mem, uint8_t half_bytes, uint64_t val)
{
  uint64_t ori = get_mem (mem, half_bytes);
  ori += val;
  set_mem (mem, half_bytes, ori);
}

// getter/setter for registers
// do not use upper 2 bit (register is 3 byte)
static uint32_t
get_reg (enum reg_flags flag)
{
  if (flag == NOT_REG)
    return __INVALID;
  if (flag >= __REGISTER_TABLE_SIZE)
    return __INVALID;
  return reg[flag];
}

static bool
set_reg (enum reg_flags flag, uint32_t val)
{
  if (flag == NOT_REG)
    return false;
  if (flag >= __REGISTER_TABLE_SIZE)
    return false;
  reg[flag] = val % __REGISTER_SIZE;
  return true;
}

static void
print_register (void)
{
  printf ("\tA : %06X X : %06X\n", reg[REG_A], reg[REG_X]);
  printf ("\tL : %06X PC: %06X\n", reg[REG_L], reg[REG_PC]);
  printf ("\tB : %06X S : %06X\n", reg[REG_B], reg[REG_S]);
  printf ("\tT : %06X\n", reg[REG_T]);
}

// initialize loader (free loader and init)
void
init_loader (void)
{
  if (bp != NULL)
    bp->cursor = 0;
  free_run ();
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

// load objects (pass2)
bool
run_loader (uint8_t *mem)
{
  if (mem == NULL)
    {
      puts ("[ERROR][DEBUG] NO mem AT run_loader");
      return false;
    }
  if (obj_list == NULL)
    {
      puts ("[ERROR][DEBUG] NO obj_list AT run_loader");
      return false;
    }

  struct q_elem *e = q_begin (obj_list);
  bool is_end_fixed = false;
  bool is_start_fixed = false;
  for (; e != q_end (obj_list); e = q_next (e))
    {
      struct prog_elem *prog =
        q_entry(e, struct prog_elem, elem);
      struct queue *obj_file = prog->obj_file;

      // start of obj addr
      int oaddr = prog->obj_addr;
      // end of obj addr
      int eaddr = oaddr + prog->obj_len;

      // refer record init
      int i;
      for (i = 0; i < 0xff; ++i)
        prog->refer[i] = -1;
      prog->refer[1] = prog->obj_addr;

      // refer record lookup
      struct q_elem *fe = q_begin (obj_file);
      for (; fe != q_end (obj_file); fe = q_next (fe))
        {
          char *line =
            q_entry(fe, struct str_elem, elem)->line;
          if (line[0] == 'R')
            {
              int i;
              int len = strlen (line);
              for (i = 1; i < len; i += 8)
                {
                  int ri = parse_chars_hex (&line[i], 2, false);
                  char *tmp = alloc_str_cpy (&line[i+2], 6);
                  prog->refer[ri] = find_sym_list (tmp);
                  free (tmp);
                  if (prog->refer[ri] < 0)
                    {
                      printf ("[%s] EXTERNAL",prog->ctrl_name);
                      printf (" SYMBOL OF R RECORD NOT FOUND\n");
                      printf ("LINE [%s] FAILED\n", line);
                    }
                }
            }
        }

      // vaddr check
      int vaddr = 0;
      fe = q_begin (obj_file);
      for (; fe != q_end (obj_file); fe = q_next (fe))
        {
          char *line =
            q_entry(fe, struct str_elem, elem)->line;
          if (line[0] == 'H')
            {
              vaddr = parse_chars_hex (line+7, 6, false);
              break;
            }
        }

      // memory input
      fe = q_begin (obj_file);
      for (; fe != q_end (obj_file); fe = q_next (fe))
        {
          char *line =
            q_entry(fe, struct str_elem, elem)->line;
          if (line[0] == 'T')
            {
              // start address for Text record
              int saddr =
                parse_chars_hex (line+1, 6, false);
              int len =
                parse_chars_hex (line+7, 2, false);
              int i;
              for (i=0;i<len;++i)
                {
                  uint8_t u =
                    parse_chars_hex (line+i*2+9, 2, false);
                  int raddr = saddr - vaddr + oaddr + i;
                  if (raddr >= eaddr)
                    {
                      printf ("[%s] ADDRESS",prog->ctrl_name);
                      printf (" EXCEEDS HEADER RECORD");
                      printf (" SIZE OF PROGRAM ADDRESS\n");
                      printf ("LINE [%s] FAILED\n", line);
                      return false;
                    }
                  if (raddr < oaddr)
                    {
                      printf ("[%s] ADDRESS",prog->ctrl_name);
                      printf (" IS SMALLER THAN");
                      printf (" PROGRAM ADDRESS\n");
                      printf ("LINE [%s] FAILED\n", line);
                      return false;
                    }
                  mem[raddr] = u;
                }
            }
        }

      // modify record
      fe = q_begin (obj_file);
      for (; fe != q_end (obj_file); fe = q_next (fe))
        {
          char *line =
            q_entry(fe, struct str_elem, elem)->line;
          if (line[0] == 'M')
            {
              int saddr = parse_chars_hex (line+1, 6, false);
              int raddr = saddr - vaddr + oaddr;
              if (raddr >= eaddr)
                {
                  printf ("[%s] ADDRESS",prog->ctrl_name);
                  printf (" EXCEEDS HEADER RECORD");
                  printf (" SIZE OF PROGRAM ADDRESS\n");
                  printf ("LINE [%s] FAILED\n", line);
                  return false;
                }
              if (raddr < oaddr)
                {
                  printf ("[%s] ADDRESS",prog->ctrl_name);
                  printf (" IS SMALLER THAN");
                  printf (" PROGRAM ADDRESS\n");
                  printf ("LINE [%s] FAILED\n", line);
                  return false;
                }

              int ri = parse_chars_hex (line+10, 6, false);
              int bytes = parse_chars_hex (line+7, 2, false);
              if (prog->refer[ri] < 0)
                {
                  printf ("[%s] EXTERNAL",prog->ctrl_name);
                  printf (" SYMBOL NOT FOUND\n");
                  printf ("LINE [%s] FAILED\n", line);
                  return false;
                }
              int val = prog->refer[ri];
              if (line[9] == '-')
                val = -val;
              add_mem (mem + raddr, bytes, val);
            }
        }

      // end record
      fe = q_begin (obj_file);
      for (; fe != q_end (obj_file); fe = q_next (fe))
        {
          char *line =
            q_entry(fe, struct str_elem, elem)->line;
          if (line[0] == 'E' && strlen(line) == 7)
            {
              if (is_end_fixed)
                {
                  printf ("[%s] MULTIPLE END", prog->ctrl_name);
                  printf (" RECORD START ADDRESSES\n");
                  printf ("LINE [%s] FAILED\n", line);
                  return false;
                }
              uint32_t saddr =
                parse_chars_hex (line+1, 6, false)
                + prog->obj_addr;
              if (is_start_fixed && get_startaddr () != saddr)
                {
                  printf ("[%s] MULTIPLE START", prog->ctrl_name);
                  printf (" ADDRESSES AT END RECORD\n");
                  printf ("LINE [%s] FAILED\n", line);
                  return false;
                }
              set_startaddr (saddr);
              is_start_fixed = true;
            }
        }
    }
  if (!validate_addr (get_startaddr ()))
    {
      printf ("[LOADER] START ADDRESS [%05X] INVALID\n",
              get_startaddr ());
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
  printf ("   \t\t    \t\ttotal length\t%04X\n", get_proglen ());
}

// check if address has to be stopped by breakpoint
uint32_t
check_bp (uint32_t addr, uint32_t len)
{
  if (bp == NULL)
    return __INVALID;
  while (bp->cursor < bp->len && bp->addr[bp->cursor] < addr)
    bp->cursor ++;
  if (bp->cursor == bp->len)
    return __INVALID;
  if (bp->addr[bp->cursor] >= addr
      && bp->addr[bp->cursor] < addr + len)
    return bp->addr[bp->cursor ++];
  return __INVALID;
}

// add another bp
bool
add_bp (uint32_t addr)
{
  if (bp == NULL)
    {
      bp = malloc (sizeof (struct breakpoint));
      if (bp == NULL)
        {
          puts ("[BREAKPOINT] MEMORY INSUFFICIENT");
          return false;
        }
      bp->addr = malloc (sizeof (int));
      if (bp->addr == NULL)
        {
          free (bp);
          puts ("[BREAKPOINT] MEMORY INSUFFICIENT");
          return false;
        }
      bp->addr[0] = addr;
      bp->len = 1;
      bp->cursor = 0;
    }
  else
    {
      int i, curr;
      for (i = 0; i < bp->len; ++i)
        if (bp->addr[i] >= addr)
          break;
      if (i < bp->len && bp->addr[i] == addr)
        {
          puts ("[BREAKPOINT] DUPLICATE BREAKPOINT");
          return false;
        }
      curr = i;

      uint32_t *tmp =
        realloc (bp->addr, (bp->len + 1) * sizeof (uint32_t));
      if (tmp == NULL)
        {
          free (bp->addr);
          free (bp);
          puts ("[BREAKPOINT] MEMORY INSUFFICIENT");
          return false;
        }
      bp->addr = tmp;
      bp->len += 1;
      for (i = bp->len - 1; i > curr; --i)
        bp->addr[i] = bp->addr[i-1];
      bp->addr[curr] = addr;
    }
  return true;
}

// print breakpoints
void
print_bp (void)
{
  puts ("\tbreakpoint");
  puts ("\t----------");

  if (bp == NULL)
    {
      puts ("\t(none)");
      return;
    }
  int i;
  for (i = 0; i < bp->len; ++i)
    printf ("\t%X\n", bp->addr[i]);
}

// free breakpoints
void
free_bp (void)
{
  if (bp != NULL)
    {
      if (bp->addr != NULL)
        free (bp->addr);
      free (bp);
      bp = NULL;
    }
}

bool
init_run (void)
{
  if (reg == NULL)
    reg = malloc (__REGISTER_TABLE_SIZE * sizeof (uint32_t));
  if (reg == NULL)
    {
      puts ("[RUN] MEMORY INSUFFICIENT");
      return false;
    }

  reg[REG_L] = __BOOT_ADDRESS;
  reg[REG_PC] = get_startaddr ();
  return true;
}

// check if two registers are valid for specific opcode
static bool
check_register (uint8_t opcode, uint8_t r1, uint8_t r2)
{
  // CLEAR, TIXR has only one operand
  if (opcode == 0xB4 || opcode == 0xB8)
    {
      if (r2 != 0)
        {
          printf ("r2 register is set, ");
          printf ("but it is not in operand\n");
          return false;
        }
    }
  // COMPR has both operand
  if (opcode == 0xA0)
    {
      if (get_reg (r2) == __INVALID)
        {
          printf ("No such register at r2.\n");
          return false;
        }
    }

  if (get_reg (r1) == __INVALID)
    {
      printf ("No such register at r1.\n");
      return false;
    }
  return true;
}

// check the memory addr for specific opcode
static bool
check_address (uint8_t opcode, uint32_t addr, bool is_i)
{
  if (is_i)
    return true;
  // J, JEQ, JLT, JSUB, RSUB, WD does not reference memory
  if (opcode == 0x3C || opcode == 0x30 || opcode == 0x38 ||
      opcode == 0x48 || opcode == 0x4C || opcode == 0xDC)
    return true;
  // COMP, LDA, LDB, LDT, STA, STL, STX reference
  // three adjacent cells
  if (opcode == 0x88 || opcode == 0x00 || opcode == 0x68 ||
      opcode == 0x74 || opcode == 0x0C || opcode == 0x14 ||
      opcode == 0x10)
    return validate_addr (addr) && validate_addr (addr + 2);
  return validate_addr (addr);
}

// get value from memory for specific opcode
static uint32_t
get_value (uint8_t opcode, uint8_t *mem)
{
  if (opcode == 0x4C)
    return 0x0;
  if (opcode == 0x88 || opcode == 0x00 || opcode == 0x68 ||
      opcode == 0x74 || opcode == 0x0C || opcode == 0x14 ||
      opcode == 0x10)
    return get_mem (mem, 6);
  return get_mem (mem, 2);
}

// compare two values
// returns 3 byte (upper 2 bit are always 0)
static uint32_t
cmp (uint32_t l, uint32_t r)
{
  l %= __REGISTER_SIZE;
  r %= __REGISTER_SIZE;
  if (l > r) return 1;
  else if (l == r) return 0;

  // l < r
  l = -1;
  l %= __REGISTER_SIZE;
  return l;
}

// parse t from unsigned to sign
// len is half byte length (value is dependent with length)
static int
parse_uint (uint32_t t, int len)
{
  char tmp[16];
  int tmplen;
  sprintf (tmp, "%x", t);
  tmplen = strlen (tmp);
  if (len > tmplen)
    {
      int diff = len - tmplen;
      int i;
      for (i=len; i>=0; --i)
        tmp[i+diff] = tmp[i];
      for (i=0; i<diff; ++i)
        tmp[i] = '0';
    }
  else if (len < tmplen)
    {
      int diff = tmplen - len;
      int i;
      for (i=diff; i<=len; ++i)
        tmp[i-diff] = tmp[i];
    }
  return parse_chars_hex (tmp, strlen (tmp), true);
}

// running functions for current loaded program
void
run (uint8_t *mem)
{
  while (true)
    {
      // fetch & decode
      uint32_t curr_addr = get_reg (REG_PC);
      if (curr_addr == __BOOT_ADDRESS)
        {
          print_register ();
          puts ("End Program");
          return ;
        }
      if (!validate_addr (curr_addr))
        {
          printf ("Segmentation Fault at %06X\n", curr_addr);
          return ;
        }

      uint8_t opcode = get_mem (&mem[curr_addr], 2);
      bool is_sic = (opcode % 4 == 0);
      bool is_immediate = (opcode % 4 == 1);
      bool is_indirect = (opcode % 4 == 2);
      opcode = (opcode / 4) * 4;

      // FORMAT 1/2
      if (is_sic)
        {
          enum format_flags fflag = find_opformat (opcode);
          if (fflag != FORMAT_1 && fflag != FORMAT_2)
            {
              printf ("Corrupted instruction at %06X\n",
                      curr_addr);
              printf ("Is not SIC instruction, but ");
              printf ("n flag and i flag are not both on.\n");
              return ;
            }

          // FORMAT 1
          if (fflag == FORMAT_1)
            {
              // check for breakpoints
              uint32_t point = check_bp (curr_addr, 1);
              if (point != __INVALID)
                {
                  print_register ();
                  printf ("Stop at checkpoint[%x]\n", point);
                  break;
                }
              set_reg (REG_PC, get_reg (REG_PC) + 1);
            }
          // FORMAT 2
          else
            {
              // check for breakpoints
              uint32_t point = check_bp (curr_addr, 2);
              if (point != __INVALID)
                {
                  print_register ();
                  printf ("Stop at checkpoint[%x]\n", point);
                  break;
                }
              set_reg (REG_PC, get_reg (REG_PC) + 2);
              uint8_t r1 = get_mem (&mem[curr_addr+1], 2) / 16;
              uint8_t r2 = get_mem (&mem[curr_addr+1], 2) % 16;

              if (!check_register (opcode, r1, r2))
                {
                  printf ("Corrupted instruction at %06X\n",
                          curr_addr);
                  return ;
                }

              // execute
              // CLEAR r1
              if (opcode == 0xB4)
                {
                  set_reg (r1, 0);
                }

              // COMPR r1, r2
              if (opcode == 0xA0)
                {
                  set_reg (REG_SW, cmp (get_reg (r1),
                                        get_reg (r2)));
                }

              // TIXR r1
              if (opcode == 0xB8)
                {
                  set_reg (REG_X, get_reg (REG_X) + 1);
                  set_reg (REG_SW, cmp (get_reg (REG_X),
                                        get_reg (r1)));
                }
            }
        }
      // FORMAT 3/4
      else
        {
          enum format_flags fflag = find_opformat (opcode);
          if (fflag != FORMAT_3)
            {
              printf ("Corrupted instruction at %06X\n",
                      curr_addr);
              printf ("n flag or i flag are on, but it is ");
              printf ("not Format 3/4 SIC/XE Instruction.\n");
              return ;
            }

          uint8_t xbpe = get_mem (&mem[curr_addr + 1], 2) / 16;
          bool is_x = (xbpe >= 8);
          bool is_b = ((xbpe % 8) >= 4);
          bool is_p = ((xbpe % 4) >= 2);
          bool is_e = xbpe % 4 == 1;
          uint32_t addr = 0x0;
          uint32_t value = 0x0;

          if (is_e)
            fflag = FORMAT_4;

          // check for breakpoints
          if (!is_e)
            {
              uint32_t point = check_bp (curr_addr, 3);
              if (point != __INVALID)
                {
                  print_register ();
                  printf ("Stop at checkpoint[%x]\n", point);
                  break;
                }
            }
          else
            {
              uint32_t point = check_bp (curr_addr, 4);
              if (point != __INVALID)
                {
                  print_register ();
                  printf ("Stop at checkpoint[%x]\n", point);
                  break;
                }
            }

          if (is_x)
            addr += parse_uint (get_reg (REG_X), 6);
          // FORMAT 3
          if (fflag == FORMAT_3)
            {
              set_reg (REG_PC, get_reg (REG_PC) + 3);

              if (is_b && is_p)
                {
                  printf ("Corrupted instruction at %06X\n",
                          curr_addr);
                  printf ("b flag and p flag are both on.\n");
                  return ;
                }

              addr +=
                parse_uint (get_mem (&mem[curr_addr+1],3), 3);
              if (is_b)
                addr += parse_uint (get_reg (REG_B), 6);
              if (is_p)
                addr += parse_uint (get_reg (REG_PC), 6);
            }
          // FORMAT 4
          else
            {
              set_reg (REG_PC, get_reg (REG_PC) + 4);
              if (is_b || is_p)
                {
                  printf ("Corrupted instruction at %06X\n",
                          curr_addr);
                  puts("b flag or p flag are set on format 4");
                  return ;
                }
              addr +=
                parse_uint (get_mem (&mem[curr_addr+1],5), 5);
            }
          if (!check_address (opcode, addr, is_immediate))
            {
              printf ("Segmentation Fault at %06X\n", curr_addr);
              return ;
            }

          // ref once more
          if (is_indirect)
            {
              addr = parse_uint (get_mem (&mem[addr], 6), 6);
              if (!check_address (opcode, addr, false))
                {
                  printf ("Segmentation Fault at %06X\n",
                          curr_addr);
                  return ;
                }
            }

          // if not immediate, ref addr
          if (is_immediate)
            value = addr;
          else
            value = get_value (opcode, &mem[addr]);

          // execute
          // COMP
          if (opcode == 0x88)
            {
              set_reg (REG_SW, cmp (get_reg (REG_A), value));
            }

          // J
          if (opcode == 0x3C)
            {
              set_reg (REG_PC, addr);
            }

          // JEQ
          if (opcode == 0x30)
            {
              if (cmp (get_reg (REG_SW), 0) == 0)
                set_reg (REG_PC, addr);
            }

          // JLT
          if (opcode == 0x38)
            {
              if (cmp (get_reg (REG_SW), -1) == 0)
                set_reg (REG_PC, addr);
            }

          // JSUB
          if (opcode == 0x48)
            {
              set_reg (REG_L, get_reg (REG_PC));
              set_reg (REG_PC, addr);
            }

          // LDA
          if (opcode == 0x00)
            {
              set_reg (REG_A, value);
            }

          // LDB
          if (opcode == 0x68)
            {
              set_reg (REG_B, value);
            }

          // LDCH
          if (opcode == 0x50)
            {
              set_mem (((uint8_t*) &reg[REG_A]) + 3, 2, value);
              reg[REG_A] %= __REGISTER_SIZE;
            }

          // LDT
          if (opcode == 0x74)
            {
              set_reg (REG_T, value);
            }

          // RD
          if (opcode == 0x78)
            {
              set_reg (REG_A, 0);
            }

          // RSUB
          if (opcode == 0x4C)
            {
              set_reg (REG_PC, get_reg (REG_L));
            }

          // STA
          if (opcode == 0x0C)
            {
              set_mem (&mem[addr], 6, get_reg (REG_A));
            }

          // STCH
          if (opcode == 0x54)
            {
              set_mem (&mem[addr], 2, get_reg (REG_A));
            }

          // STL
          if (opcode == 0x14)
            {
              set_mem (&mem[addr], 6, get_reg (REG_L));
            }

          // STX
          if (opcode == 0x10)
            {
              set_mem (&mem[addr], 6, get_reg (REG_X));
            }

          // TD
          if (opcode == 0xE0)
            {
              set_reg (REG_SW, -1);
            }

          // WD
          if (opcode == 0xDC)
            {
            }
        }
    }
}

// free running environment
void
free_run (void)
{
  if (reg != NULL)
    free (reg);
  reg = NULL;
}
