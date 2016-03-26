#include "filectrl.h"
#include "queue.h"
#include "20141589.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

/*
bool
assemble_file (struct queue *oplist, const char *filename)
{
  if (!is_file(filename))
    return false;
  
  FILE *fp = fopen(filename, "r");
  bool is_assembled = false;


  return is_assembled;
}*/
