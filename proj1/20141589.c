#include "20141589.h"
#include "queue.h"
#include "hexctrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

// MACRO FUNCTIONS
#define _ERROR_EXIT(s,i) do{\
    perror(s);\
    exit(i);\
} while(0);
#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

#define __CMD_FORMAT_SIZE 16
#define __CMD_TABLE_SIZE 10
#define __SHORT_CMD_TABLE_SIZE 7

// FIXED VARIABLES
static const int __MEMORY_SIZE = 1<<20;
static const int __INPUT_SIZE = 64;
static const int __CMD_SIZE = 16;
static const int __TABLE_SIZE = 20;
static const char __CMD_FORMAT[__CMD_FORMAT_SIZE];
static const char *__CMD_TABLE[__CMD_TABLE_SIZE] =\
{"help", "dir", "quit", "history", "dump",\
  "edit", "fill", "reset", "opcode", "opcodelist" };
static const char *__SHORT_CMD_TABLE[__SHORT_CMD_TABLE_SIZE] =\
{"h","d","q","hi","du","e","f"};
static const char *__SHELL_FORM = "sicsim> ";
static const char *__HELP_FORM = "h[elp]\nd[ir]\nq[uit]\n\
hi[story]\ndu[mp] [start, end]\ne[dit] \
address, value\nf[ill] start, end, value\n\
reset\nopcode mnemonic\nopcodelist";


// Deletes every char input after size. 
static bool
get_chars (char *input, int size)
{
  int i = 0;
  char c;

  while ( (c=getchar()) != EOF && c != '\n' && c != '\0' )
    if(i < size-1) input[i++] = c;
  input[i] = '\0';
  while(input[i-1] == ' ')
    input[--i] = '\0';

  return c != EOF;
}

static int
get_cmd_index (char *input)
{
  int i=0;
  for (i=0; i<__CMD_TABLE_SIZE; ++i)
    if (_SAME_STR(__CMD_TABLE[i], input))
      return i;
  for (i=0; i<__SHORT_CMD_TABLE_SIZE; ++i)
    if (_SAME_STR(__SHORT_CMD_TABLE[i], input))
      return i;

  return -1;
}

static uint64_t
str_hash (char *str)
{
  // Reference: djb2 by Dan Bernstein (York Univ.)
  uint64_t c, hash = 5381;

  while ((c = *str++)!='\0')
    hash = ((hash << 5) + hash) + c;

  return hash;
}

int
main(void)
{
  // INITIALIZING
  struct queue cmd_queue;
  q_init (&cmd_queue);

  uint8_t *mem = calloc (__MEMORY_SIZE, sizeof(uint8_t));
  struct queue *oplist = malloc (sizeof(struct queue)
                                 * __TABLE_SIZE);
  char *input = malloc (sizeof(char)*__INPUT_SIZE);
  char *cmd = malloc (sizeof(char)*__CMD_SIZE);
  if (mem == NULL || input == NULL
      || cmd == NULL || oplist == NULL)
    goto memory_clear;

  // OPCODE READ
  int i;
  for (i=0; i<__TABLE_SIZE; ++i)
    q_init (&oplist[i]);

  FILE * fp = fopen("opcode.txt", "r");
  if (fp == NULL)
    goto memory_clear;

  // Formatting string
  i = snprintf((char *) __CMD_FORMAT, __CMD_FORMAT_SIZE,
               "%%hhx %%%ds %%s", __CMD_SIZE - 1);
  if (i < 0 || i > __CMD_FORMAT_SIZE)
    goto memory_clear;

  while (fgets(input, __INPUT_SIZE, fp) != NULL)
    {
      uint8_t code;
      char form[__OPCODE_FORMAT_SIZE];
      sscanf(input, (const char *) __CMD_FORMAT,
             &code, cmd, &form);
      
      // Saving opcode
      struct op_elem *oe = malloc(sizeof(struct op_elem));
      if (oe == NULL)
        goto memory_clear;
      oe->opcode = malloc(sizeof(char)*(strlen(cmd)+1));
      if(oe->opcode == NULL)
        goto memory_clear;
      strcpy(oe->opcode, cmd);
      strcpy(oe->format, form);
      oe->code = code;

      code = str_hash (cmd) % __TABLE_SIZE;
      q_insert (&oplist[code], &(oe->elem));
    }

  // COMMAND PROCESSING
  while (true)
    {
      struct q_elem *qe;
      uint8_t value;
      uint32_t start, end;

      printf("%s", __SHELL_FORM);
      get_chars(input, __INPUT_SIZE);

      // Saving commands
      struct cmd_elem *e = malloc(sizeof(struct cmd_elem));
      if (e == NULL)
        goto memory_clear;
      e->cmd = malloc(sizeof(char)*(strlen(input)+1));
      if (e->cmd == NULL)
        goto memory_clear;
      strcpy(e->cmd, input);
      q_insert (&cmd_queue, &(e->elem));
 
      // Processing input string
      snprintf((char *) __CMD_FORMAT, __CMD_FORMAT_SIZE,
                   "%%%ds", __CMD_SIZE - 1);
      sscanf(input, (const char *) __CMD_FORMAT, cmd);
      
      // Switching with commands
      switch(get_cmd_index(cmd))
        {
        case CMD_HELP:
          puts(__HELP_FORM);
          break;
        
        case CMD_DIR:
          break;
        
        case CMD_QUIT:
          goto memory_clear;
        
        case CMD_HISTORY:
          qe = q_begin (&cmd_queue);
          i = 1;
          for (; qe!=q_end(&cmd_queue); qe=q_next(qe))
            printf("%-4d %s\n", i++,
                   q_entry(qe, struct cmd_elem, elem)->cmd);
          break;
        
        case CMD_DUMP:
          switch(sscanf(input, "%s %x, %x", cmd, &start, &end))
            {
            case 1:
              autodump (mem, __MEMORY_SIZE, 0x10 * 10);
              break;
            case 2:
              get_location (start, true);
              autodump (mem, __MEMORY_SIZE, 0x10 * 10);
              break;
            case 3:
              get_location (start, true);
              autodump (mem, __MEMORY_SIZE, end - start + 1);
              break;
            }
          break;
        
        case CMD_EDIT:
          switch(sscanf(input, "%s %x, %hhx",
                        cmd, &start, &value))
            {
            case 3:
              hexfill (mem, __MEMORY_SIZE, start, start, value);
              break;
            }
          break;
        
        case CMD_FILL:
          switch(sscanf(input, "%s %x, %x, %hhx",
                        cmd, &start, &end, &value))
            {
            case 4:
              hexfill (mem, __MEMORY_SIZE, start, end, value);
              break;
            }
          break;
        
        case CMD_RESET:
          hexfill (mem, __MEMORY_SIZE, 0, __MEMORY_SIZE - 1, 0);
          break;
        
        case CMD_OPCODE:
           switch(sscanf(input, "%*s %s", cmd))
            {
            case 1:
              i = str_hash(cmd) % __TABLE_SIZE;
              if (!q_empty(&oplist[i]))
                {
                  qe = q_begin (&oplist[i]);
                  for(; qe != q_end(&oplist[i]); qe = q_next(qe))
                    {
                      struct op_elem *oe
                        = q_entry (qe, struct op_elem, elem);
                      if (_SAME_STR(cmd, oe->opcode))
                        {
                          printf("opcode is %2X\n", oe->code);
                          break;
                        }
                    }
                }
              break;
            }

          break;
        
        case CMD_OPCODELIST:
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
                      printf ("-> [%s:%02X] ",
                              oe->opcode, oe->code);
                    }
                }
              puts("");
            }
          break;
        }
    }


memory_clear:
  if (mem != NULL)
    free (mem);
  if (input != NULL)
    free (input);
  if (cmd != NULL)
    free (cmd);
  while(!q_empty(&cmd_queue))
    {
      struct q_elem *e = q_delete(&cmd_queue);
      struct cmd_elem *ce = q_entry(e, struct cmd_elem, elem);
      if (ce->cmd != NULL)
        free(ce->cmd);
      free(ce);
    }
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
  if (oplist != NULL)
    free (oplist);

  return 0;
}
