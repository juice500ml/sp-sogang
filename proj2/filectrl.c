#include "filectrl.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// MACRO FUNCTIONS
#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

static const int __TABLE_SIZE = 20;
static struct queue *oplist;
static const int __READ_SIZE = 128;

bool
is_file (const char *filename)
{
  FILE *fp = fopen(filename, "r");
  
  if (fp != NULL)
    {
      fclose(fp);
      return true;
    }
  return false;
}

bool
print_file (const char *filename)
{
  if (!is_file(filename))
    return false;
  
  FILE *fp = fopen(filename, "r");
  int c;

  while((c=fgetc(fp))!=EOF)
    putchar(c);
  return true;
}

void free_oplist (void)
{
  if (oplist == NULL)
    return;
  
  int i = 0;
  for(i=0; i<__TABLE_SIZE; ++i)
    {
      while(!q_empty(&oplist[i]))
        {
          struct q_elem *e = q_delete(&oplist[i]);
          struct op_elem *oe = q_entry(e, struct op_elem, elem);
          if (oe->opcode != NULL)
            free(oe->opcode);
          free(oe);
        }
    }
  free (oplist);
  oplist = NULL;
}

void
print_oplist (void)
{
  int i;
  struct q_elem *qe;

  // traverse through every table
  for(i=0; i<__TABLE_SIZE; ++i)
    {
      printf("%d : ", i);
      if (!q_empty(&oplist[i]))
        {
          qe = q_begin (&oplist[i]);
          struct op_elem *oe 
            = q_entry (qe, struct op_elem, elem);
          printf ("[%s:%02X] ", oe->opcode, oe->code);
          for(qe = q_next(qe); qe != q_end(&oplist[i]);
              qe = q_next(qe))
            {
              oe = q_entry (qe, struct op_elem, elem);
              printf ("-> [%s:%02X] ", oe->opcode, oe->code);
            }
        }
        puts("");
      }
}

int
find_oplist (char *cmd)
{
  // look for opcode in hash table
  int i = str_hash(cmd) % __TABLE_SIZE;
  if (!q_empty(&oplist[i]))
    {
      struct q_elem *qe = q_begin (&oplist[i]);
      for(; qe != q_end(&oplist[i]); qe = q_next(qe))
        {
          struct op_elem *oe
            = q_entry (qe, struct op_elem, elem);
          if (strcmp(cmd, oe->opcode) == 0)
            return oe->code;
        }
    }
  return -1;
}

bool
init_oplist (const char *filename)
{
  int i;
  char *input = NULL;
  free_oplist ();
  
  FILE * fp = fopen(filename, "r");
  if (fp == NULL)
    {
      printf("[OPCODE] %s NOT FOUND\n", filename);
      goto memory_clear;
    }

  oplist = malloc (sizeof(struct queue)*__TABLE_SIZE);
  input = malloc(sizeof(char)*__READ_SIZE);
  if (oplist == NULL || input == NULL)
    {
      puts("[OPCODE] MEMORY INSUFFICIENT");
      goto memory_clear;
    }

  for (i=0; i<__TABLE_SIZE; ++i)
    q_init (&oplist[i]);

  // opcode hash table generation
  while (fgets(input, __READ_SIZE, fp) != NULL)
    {
      uint8_t code = 0;
      char cmd[16] = {0, };
      char frm[16] = {0, };

      if (sscanf(input, "%*x %*15s %*15s %15s", frm) == 1
          || sscanf(input,
                    "%hhx %15s %15s",
                    &code, cmd, frm) != 3)
        {
          printf("[OPCODE] %s IS BROKEN\n", filename);
          goto memory_clear;
        }
      
      // Saving opcode
      struct op_elem *oe = malloc(sizeof(struct op_elem));
      if (oe == NULL)
        {
          puts("[OPCODE] MEMORY INSUFFICIENT");
          goto memory_clear;
        }
      oe->opcode = malloc(sizeof(char)*(strlen(cmd)+1));
      if(oe->opcode == NULL)
        {
          puts("[OPCODE] MEMORY INSUFFICIENT");
          goto memory_clear;
        }
      strcpy(oe->opcode, cmd);
      strcpy(oe->format, frm);
      oe->code = code;

      code = str_hash (cmd) % __TABLE_SIZE;
      q_insert (&oplist[code], &(oe->elem));
    }
  return true;

memory_clear:
  free_oplist ();
  if (input != NULL)
    free (input);

  return false;
}

// get string from fp
static bool
get_str (char *input, int size, FILE *fp)
{
  if (fgets (input, size, fp) == NULL)
    return false;

  int i = strlen(input) - 1;
  while (i>=0 && (input[i] == '\n'
                  || input[i] == '\t'
                  || input[i] == ' '))
    input[i--] = '\0';

  // if empty string, get next string
  if (strlen(input) == 0)
    return get_str (input, size, fp);
  return true;
}

// input string in queue
static bool
insert_mid_str (int line, uint32_t ctr, char *s, struct queue *q)
{
  struct mid_elem *me = malloc(sizeof(struct mid_elem));
  if (me == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      return false;
    }
  // additional line & ctr
  me->line = malloc(sizeof(char)*(strlen(s)+20));
  if (me->line == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      return false;
    }
  sprintf (me->line, "%4d %04X %s", line, ctr, s);
  q_insert (q, &(me->elem));
  printf("[%s] inserted!\n", me->line);
  return true;
}

// input symbol in queue
static bool
insert_sym_str (char *label, int ctr, struct queue tbl[])
{
  struct sym_elem *se = malloc(sizeof(struct sym_elem));
  if (se == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      return false;
    }
  se->label = malloc(sizeof(char)*(strlen(label)+1));
  if (se->label == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      return false;
    }
  strcpy (se->label, label);
  se->locctr = ctr;
  uint64_t i = str_hash (se->label) % __TABLE_SIZE;
  q_insert (&(tbl[i]), &(se->elem));
  printf("[%s][%x] inserted!\n", se->label, se->locctr);
  return true;
}

bool
assemble_file (const char *filename)
{
  FILE *fp = fopen(filename, "r");
  bool is_assembled = false;
  char *input = NULL;
  char *cmd = NULL;
  char chk[2];
  uint32_t locctr = 0;
  uint32_t startaddr = 0;
  int linenum = 0;
  int i = 0;
  struct queue mid_queue;
  struct queue sym_table[__TABLE_SIZE];

  q_init (&mid_queue);
  for (i=0; i<__TABLE_SIZE; ++i)
    q_init (&sym_table[i]);

  if (fp == NULL)
    {
      printf("[ASSEMBLER] %s NOT FOUND\n", filename);
      goto end_assemble;
    }
  
  input = malloc(sizeof(char)*__READ_SIZE);
  cmd = malloc(sizeof(char)*__READ_SIZE);
  if (input == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      goto end_assemble;
    }
  
  // check if empty
  if (!get_str(input, __READ_SIZE, fp))
    {
      printf("[ASSEMBLER] %s IS EMPTY\n", filename);
      goto end_assemble;
    }
  
  // START
  // ex) START 1000
  if (sscanf(input, "%s", cmd)==1 && _SAME_STR(cmd, "START"))
    {
      if (sscanf(input, "%*s %s %1s", cmd, chk) == 1
          && sscanf(cmd, "%x %1s", &startaddr, chk) == 1)
        {
          locctr = startaddr;
          linenum += 5;
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue))
            goto end_assemble;
        }
      else
        {
          printf("[ASSEMBLER] SYNTAX ERROR: [%s]\n", input);
          printf("[ASSEMBLER] WARNING: SET DEFAULT START ");
          printf("ADDR TO 0x0000\n");
        }
    }
  // ex) COPY START 1000
  else if (sscanf(input, "%*s %s", cmd)==1
           && _SAME_STR(cmd, "START"))
    {
      if (sscanf(input, "%*s %*s %s %1s", cmd, chk) == 1
          && sscanf(cmd, "%x %1s", &startaddr, chk) == 1)
        {
          locctr = startaddr;
          linenum += 5;
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue))
            goto end_assemble;
          
          sscanf(input, "%s", cmd);
          if (!insert_sym_str (cmd, locctr, sym_table))
            goto end_assemble;
        }
      else
        {
          printf("[ASSEMBLER] SYNTAX ERROR: [%s]\n", input);
          printf("[ASSEMBLER] WARNING: SET DEFAULT START");
          printf(" ADDR TO 0x0000\n");
        }
    }
  // nothing
  else
    {
      rewind (fp);
    }

  // lst generation
  do
    {
      // read line until valid
      if (!get_str(input, __READ_SIZE, fp))
        {
          printf("[ASSEMBLER] UNEXPECTED EOF\n");
          goto end_assemble;
        }

      // check if comment
      if (input[0] == '.')
        continue;

    }
  while (true);

  is_assembled = true;

end_assemble:
  if (fp != NULL)
    fclose (fp);
  if (input != NULL)
    free (input);

  // if not assembled free
  while (!q_empty(&mid_queue))
    {
      struct q_elem *e = q_delete (&mid_queue);
      struct mid_elem *me = q_entry (e, struct mid_elem, elem);
      if (me->line != NULL)
        free (me->line);
      free (me);
    }
  // else save mid file and free
  for (i=0; i<__TABLE_SIZE; ++i)
    {
      while (!q_empty(&(sym_table[i])))
        {
          struct q_elem *e = q_delete (&(sym_table[i]));
          struct sym_elem *se = q_entry (e, struct sym_elem, elem);
          if (se->label != NULL)
            free (se->label);
          free (se);
        }
    }

  return is_assembled;
}
