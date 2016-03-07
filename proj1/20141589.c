#include "20141589.h"
#include "queue.h"

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
static const char *__HELP_FORM = "h[elp]\nd[ir]\nq[uit]\nhi[story]\n\
du[mp] [start, end]\ne[dit] address, \
value\nf[ill] start, end, value\nreset\n\
opcode mnemonic\nopcodelist\n";


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

  char *input = malloc (sizeof(char)*__INPUT_SIZE);
  char *cmd = malloc (sizeof(char)*__CMD_SIZE);
  if (input == NULL || cmd == NULL)
    goto memory_clear;

  // COMMAND PROCESSING
  while (true)
    {
      struct q_elem *qe = NULL;
      int i = 0;

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
 
      // Formatting string
      i = snprintf((char *) __CMD_FORMAT, __CMD_FORMAT_SIZE, "%%%ds", __CMD_SIZE-1);
      if (i < 0 || i > __CMD_FORMAT_SIZE)
        goto memory_clear;

      sscanf(input, (const char *) __CMD_FORMAT, cmd);
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
            printf("%-4d %s\n", i++, q_entry(qe, struct cmd_elem, elem)->cmd);
          break;
        
        case CMD_DUMP:
          break;
        
        case CMD_EDIT:
          break;
        
        case CMD_FILL:
          break;
        
        case CMD_RESET:
          break;
        
        case CMD_OPCODE:
          break;
        
        case CMD_OPCODELIST:
          break;
        }
    }


memory_clear:
  if (input != NULL)
    free (input);
  if (cmd != NULL)
    free (cmd);
  // TODO: free cmd_queue

  return 0;
}
