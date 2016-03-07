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

#define __CMD_FORMAT_SIZE 8
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
get_chars(char *input, int size)
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
get_cmd_index(char *input)
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

int
main(void)
{
  // INITIALIZING
  struct queue cmd_queue;
  q_init(&cmd_queue);

  uint8_t *mem = calloc(__MEMORY_SIZE, sizeof(uint8_t));
  char *input = malloc (sizeof(char)*__INPUT_SIZE);
  char *cmd = malloc (sizeof(char)*__CMD_SIZE);
  if (mem == NULL || input == NULL || cmd == NULL)
    goto memory_clear;

  // COMMAND PROCESSING
  while (true)
    {
      struct q_elem *qe;
      int i;
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
      // Formatting string
      i = snprintf((char *) __CMD_FORMAT,
                   __CMD_FORMAT_SIZE, "%%%ds", __CMD_SIZE-1);
      if (i < 0 || i > __CMD_FORMAT_SIZE)
        goto memory_clear;
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
            }
          break;
        
        case CMD_FILL:
          switch(sscanf(input, "%s %x, %x, %hhx",
                        cmd, &start, &end, &value))
            {
            case 4:
              hexfill (mem, __MEMORY_SIZE, start, end, value);
            }
          break;
        
        case CMD_RESET:
          hexfill (mem, __MEMORY_SIZE, 0, __MEMORY_SIZE - 1, 0);
          break;
        
        case CMD_OPCODE:
          break;
        
        case CMD_OPCODELIST:
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
      free(ce->cmd);
      free(ce);
    }

  return 0;
}
