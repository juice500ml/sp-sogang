#include "20141589.h"
#include "queue.h"
#include "hexctrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>

// MACRO FUNCTIONS
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
static const char *__OPCODE_FILENAME = "opcode.txt";
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
  int i, c;

  i = 0;
  while ( (c=getchar()) != EOF && c != '\n' && c != '\0' )
    if(i < size-1) input[i++] = c;
  input[i] = '\0';

  if(i>0)
    while(input[i-1] == ' ' || input[i-1] == '\t')
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
    {
      puts("MEMORY INSUFFICIENT.");
      goto memory_clear;
    }

  // OPCODE READ
  int i;
  for (i=0; i<__TABLE_SIZE; ++i)
    q_init (&oplist[i]);

  FILE * fp = fopen(__OPCODE_FILENAME, "r");
  if (fp == NULL)
    {
      printf("%s NOT FOUND.\n", __OPCODE_FILENAME);
      goto memory_clear;
    }

  // Formatting string
  i = snprintf((char *) __CMD_FORMAT, __CMD_FORMAT_SIZE,
               "%%hhx %%%ds %%s", __CMD_SIZE - 1);
  if (i < 0 || i > __CMD_FORMAT_SIZE)
    {
      puts("COMMAND SIZE IS TOO BIG.");
      goto memory_clear;
    }

  // opcode hash table generation
  while (fgets(input, __INPUT_SIZE, fp) != NULL)
    {
      uint8_t code;
      char form[__OPCODE_FORMAT_SIZE];
      if (sscanf(input, (const char *) __CMD_FORMAT,
             &code, cmd, &form) != 3)
        {
          printf("%s is broken.\n", __OPCODE_FILENAME);
          goto memory_clear;
        }
      
      // Saving opcode
      struct op_elem *oe = malloc(sizeof(struct op_elem));
      if (oe == NULL)
        {
          puts("MEMORY INSUFFICIENT.");
          goto memory_clear;
        }
      oe->opcode = malloc(sizeof(char)*(strlen(cmd)+1));
      if(oe->opcode == NULL)
        {
          puts("MEMORY INSUFFICIENT.");
          goto memory_clear;
        }
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
      DIR *dirp = NULL;
      struct dirent *dir = NULL;
      char check[2];

      printf("%s", __SHELL_FORM);
      if (!get_chars(input, __INPUT_SIZE))
        goto memory_clear;

      // Saving commands
      struct cmd_elem *e = malloc(sizeof(struct cmd_elem));
      if (e == NULL)
        {
          puts("MEMORY INSUFFICIENT.");
          goto memory_clear;
        }
      e->cmd = malloc(sizeof(char)*(strlen(input)+1));
      if (e->cmd == NULL)
        {
          puts("MEMORY INSUFFICIENT.");
          goto memory_clear;
        }
      strcpy(e->cmd, input);
      q_insert (&cmd_queue, &(e->elem));
 
      // Processing input string
      snprintf((char *) __CMD_FORMAT, __CMD_FORMAT_SIZE,
                   "%%%ds", __CMD_SIZE - 1);
      if (sscanf(input, (const char *) __CMD_FORMAT, cmd)!=1)
        cmd[0] = '\0';
      
      // Switching with commands
      switch(get_cmd_index(cmd))
        {
        case CMD_HELP:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          puts(__HELP_FORM);
          break;
        
        case CMD_DIR:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          i = 1;
          dirp = opendir(".");
          dir = readdir(dirp);
          for(; dir!=NULL; dir = readdir(dirp))
            {
              struct stat st;
              if(stat((const char*) dir->d_name, &st)!=0)
                {
                  puts("FILE NOT FOUND");
                  goto memory_clear;
                }
              if(_SAME_STR(dir->d_name, ".")
                 || _SAME_STR(dir->d_name, ".."))
                continue;
              printf("%s", dir->d_name);
              if(S_ISDIR(st.st_mode))
                putchar('/');
              else if( (st.st_mode & S_IXUSR)
                 || (st.st_mode & S_IXGRP)
                 || (st.st_mode & S_IXOTH) )
                putchar('*');
              printf("   ");
              
              if((i++)%5==0)
                putchar('\n');
            }
          if((i-1)%5!=0)
            putchar('\n');
          break;
        
        case CMD_QUIT:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          goto memory_clear;
        
        case CMD_HISTORY:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          qe = q_begin (&cmd_queue);
          i = 1;
          for (; qe!=q_end(&cmd_queue); qe=q_next(qe))
            printf("%-4d %s\n", i++,
                   q_entry(qe, struct cmd_elem, elem)->cmd);
          break;
        
        case CMD_DUMP:
          switch(sscanf(input, "%s %x , %x", cmd, &start, &end))
            {
            case 1:
              if(sscanf(input, "%*s %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
              autodump (mem, __MEMORY_SIZE, 0x10 * 10);
              break;
            
            case 2:
              if(sscanf(input, "%*s %*x %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
              if (!(start < __MEMORY_SIZE))
                {
                  puts("OUT OF MEMORY BOUNDS.");
                  break;
                }
              get_location (start, true);
              autodump (mem, __MEMORY_SIZE, 0x10 * 10);
              break;
            
            case 3:
              if(sscanf(input, "%*s %*x , %*x %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
              if (!(start<=end && end<__MEMORY_SIZE))
                {
                  puts("OUT OF MEMORY BOUNDS.");
                  break;
                }
              get_location (start, true);
              autodump (mem, __MEMORY_SIZE, end - start + 1);
              break;

            default:
              puts("WRONG INSTRUCTION");
              break;
            }
          break;
        
        case CMD_EDIT:
          switch(sscanf(input, "%s %x , %hhx",
                        cmd, &start, &value))
            {
            case 3:
              if(sscanf(input, "%*s %*x , %*x %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
              hexfill (mem, __MEMORY_SIZE, start, start, value);
              break;
            
            default:
              puts("WRONG INSTRUCTION");
              break;
            }
          break;
        
        case CMD_FILL:
          switch(sscanf(input, "%s %x , %x , %hhx",
                        cmd, &start, &end, &value))
            {
            case 4:
              if(sscanf(input,
                        "%*s %*x , %*x , %*x %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
              hexfill (mem, __MEMORY_SIZE, start, end, value);
              break;
            
            default:
              puts("WRONG INSTRUCTION");
              break;
            }
          break;

        case CMD_RESET:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          hexfill (mem, __MEMORY_SIZE, 0, __MEMORY_SIZE - 1, 0);
          break;

        case CMD_OPCODE:
          switch(sscanf(input, "%*s %s", cmd))
            {
            case 1:
              if(sscanf(input, "%*s %*s %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
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

            default:
              puts("WRONG INSTRUCTION");
              break;
            }
          break;

        case CMD_OPCODELIST:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
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

        default:
          if(sscanf(input, "%1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
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
