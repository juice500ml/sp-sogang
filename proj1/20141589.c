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

// FIXED VARIABLES
static const int __INPUT_SIZE = 64;
static const int __TABLE_SIZE = 20;
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
  if (_SAME_STR("h", input)) return CMD_HELP;
  if (_SAME_STR("help", input)) return CMD_HELP;
  if (_SAME_STR("d", input)) return CMD_DIR;
  if (_SAME_STR("dir", input)) return CMD_DIR;
  if (_SAME_STR("q", input)) return CMD_QUIT;
  if (_SAME_STR("quit", input)) return CMD_QUIT;
  if (_SAME_STR("hi", input)) return CMD_HISTORY;
  if (_SAME_STR("history", input)) return CMD_HISTORY;
  if (input[0] == 'd' && input[1] == 'u'
      && (input[2] == ' ' || input[2] == '\0')) return CMD_DUMP;
  if (input[0] == 'd' && input[1] == 'u'
      && input[2] == 'm' && input[3] == 'p'
      && (input[4] == ' ' || input[4] == '\0')) return CMD_DUMP;
  if (input[0] == 'e' && input[1] == ' ') return CMD_EDIT;
  if (input[0] == 'e' && input[1] == 'd'
      && input[2] == 'i' && input[3] == 't'
      && input[4] == ' ') return CMD_EDIT;
  if (input[0] == 'f' && input[1] == ' ') return CMD_FILL;
  if (input[0] == 'f' && input[1] == 'i'
      && input[2] == 'l' && input[3] == 'l'
      && input[4] == ' ') return CMD_FILL;
  if (_SAME_STR("reset", input)) return CMD_RESET;
  if (input[0] == 'o' && input[1] == 'p'
      && input[2] == 'c' && input[3] == 'o'
      && input[4] == 'd' && input[5] == 'e'
      && input[6] == ' ') return CMD_OPCODE;
  if (_SAME_STR("opcodelist", input)) return CMD_OPCODELIST;

  return -1;
}

int
main(void)
{
  // INITIALIZING
  struct queue cmd_queue;
  q_init(&cmd_queue);

  char *input = malloc (sizeof(char)*__INPUT_SIZE);
  if (input == NULL)
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
      e->cmd = input;
      q_insert (&cmd_queue, &(e->elem));
      
      switch(get_cmd_index(input))
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
            printf("%4d %s\n", i++, q_entry(qe, struct cmd_elem, elem)->cmd);
          break;
        }

      // Allocate new input string
      input = malloc (sizeof(char)*__INPUT_SIZE);
      if (input == NULL)
        goto memory_clear;
    }


memory_clear:
  free(input);

  return 0;
}
