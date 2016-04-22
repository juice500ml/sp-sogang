#include "20141589.h"
#include "queue.h"
#include "hexctrl.h"
#include "filectrl.h"
#include "objctrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>

// MACRO FUNCTIONS
#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

#define __CMD_FORMAT_SIZE 32
#define __CMD_TABLE_SIZE 17
#define __SHORT_CMD_TABLE_SIZE 7

// FIXED VARIABLES
static const char *__OPCODE_FILENAME = "opcode.txt";
static const int __MEMORY_SIZE = 1<<20;
static const int __INPUT_SIZE = 128;
static const int __CMD_SIZE = 128;
static const int __FILENAME_SIZE = 128;
static const char __CMD_FORMAT[__CMD_FORMAT_SIZE];
static const char *__CMD_TABLE[__CMD_TABLE_SIZE] =\
{"help", "dir", "quit", "history", "dump",\
  "edit", "fill", "reset", "opcode", "opcodelist",\
  "assemble", "type", "symbol", "progaddr", "loader",\
  "run", "bp"};
static const char *__SHORT_CMD_TABLE[__SHORT_CMD_TABLE_SIZE] =\
{"h","d","q","hi","du","e","f"};
static const char *__SHELL_FORM = "sicsim> ";
static const char *__HELP_FORM = "h[elp]\nd[ir]\nq[uit]\n\
hi[story]\ndu[mp] [start, end]\ne[dit] \
address, value\nf[ill] start, end, value\n\
reset\nopcode mnemonic\nopcodelist\n\
assemble filename\ntype filename\nsymbol\n\
progaddr [address]\nloader [object filename1] \
[object filename2] [...]\nrun\nbp";


// Deletes every char input after size.
static bool
get_chars (char *input, int size)
{
  int i, c;

  i = 0;
  while ( (c=getchar()) != EOF && c != '\n' && c != '\0' )
    if(i < size-1) input[i++] = c;
  input[i] = '\0';

  // delete trailing whitespace
  if(i>0)
    while(input[i-1] == ' ' || input[i-1] == '\t')
      input[--i] = '\0';

  return c != EOF;
}

// Get command index (same with enum cmd_flags) 
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

int
main(void)
{
  // INITIALIZING
  struct queue cmd_queue;
  q_init (&cmd_queue);

  uint8_t *mem = calloc (__MEMORY_SIZE, sizeof(uint8_t));
  char *input = malloc (sizeof(char)*__INPUT_SIZE);
  char *cmd = malloc (sizeof(char)*__CMD_SIZE);
  char *filename = malloc (sizeof(char)*__FILENAME_SIZE);
  if (mem == NULL || input == NULL || filename == NULL
      || cmd == NULL)
    {
      puts("MEMORY INSUFFICIENT");
      goto memory_clear;
    }

  if (!init_oplist (__OPCODE_FILENAME))
    {
      puts("OPCODE LIST INITIALIZATION FAILED.");
      goto memory_clear;
    }

  // COMMAND PROCESSING
  while (true)
    {
      int i;
      struct q_elem *qe;
      uint8_t value;
      uint32_t start, end;
      DIR *dirp = NULL;
      struct dirent *dir = NULL;
      char check[2];
      bool is_valid_cmd = false;

      printf("%s", __SHELL_FORM);
      if (!get_chars(input, __INPUT_SIZE))
        goto memory_clear;

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
          is_valid_cmd = true;
          break;
        
        case CMD_DIR:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          // open directory and read through all elem.
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
              // FIX: ignore . and ..
              if(_SAME_STR(dir->d_name, ".")
                 || _SAME_STR(dir->d_name, ".."))
                continue;
              printf("%20s", dir->d_name);
              if(S_ISDIR(st.st_mode)) // is Directory?
                putchar('/');
              else if( (st.st_mode & S_IXUSR) // is exe?
                 || (st.st_mode & S_IXGRP)
                 || (st.st_mode & S_IXOTH) )
                putchar('*');
              putchar('\t');
             
              // print newline after 3 elements
              if((i++)%3==0)
                putchar('\n');
            }
          if((i-1)%3!=0)
            putchar('\n');
          
          is_valid_cmd = true;
          break;
        
        case CMD_QUIT:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          
          is_valid_cmd = true;
          goto memory_clear;
        
        case CMD_HISTORY:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          qe = q_begin (&cmd_queue);
          i = 1;
          // print every formatted history
          for (; qe!=q_end(&cmd_queue); qe=q_next(qe))
            printf("%-4d %s\n", i++,
                   q_entry(qe, struct cmd_elem, elem)->cmd);
          printf("%-4d %s\n", i, input);
          
          is_valid_cmd = true;
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
              start = get_location (0, false);
              end = start + 0x10 * 10 - 1;
              // if end is too large, point to end and go 0
              if ( end >= __MEMORY_SIZE )
                end = __MEMORY_SIZE - 1;
              hexdump (mem, start, end);
              if ( end == __MEMORY_SIZE - 1)
                get_location (0, true);
              else
                get_location (end + 1, true);
              
              is_valid_cmd = true;
              break;
            
            case 2:
              if(sscanf(input, "%*s %*x %1s", check) == 1)
                {
                  puts("WRONG INSTRUCTION");
                  break;
                }
              if (start >= __MEMORY_SIZE)
                {
                  puts("OUT OF MEMORY BOUNDS.");
                  break;
                }
              end = start + 0x10 * 10 - 1;
              // if end is too large, point to end and go 0
              if ( end >= __MEMORY_SIZE )
                end = __MEMORY_SIZE - 1;
              hexdump (mem, start, end);
              if ( end == __MEMORY_SIZE - 1)
                get_location (0, true);
              else
                get_location (end + 1, true);
              
              is_valid_cmd = true;
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
              hexdump (mem, start, end);
              // if end is too large, point to end and go 0
              if ( end == __MEMORY_SIZE - 1)
                get_location (0, true);
              else
                get_location (end + 1, true);
              
              is_valid_cmd = true;
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
              
              is_valid_cmd = true;
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
              
              is_valid_cmd = true;
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
          // equivalent to fill 0, __MEMORY_SIZE-1
          hexfill (mem, __MEMORY_SIZE, 0, __MEMORY_SIZE - 1, 0);
              
          is_valid_cmd = true;
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
              i = find_oplist (cmd);
              if (i != -1)
                printf("opcode is %2X\n", i);
              else
                {
                  printf("%s: NO SUCH OPCODE\n", cmd);
                  is_valid_cmd = false;
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
          print_oplist ();
          is_valid_cmd = true;
          break;

        case CMD_ASSEMBLE:
          // Processing input string
          snprintf((char *) __CMD_FORMAT,
                   __CMD_FORMAT_SIZE,
                   "%%%ds %%%ds %%1s",
                   __CMD_SIZE - 1,
                   __FILENAME_SIZE - 1);
          if (sscanf(input,
                     (const char *) __CMD_FORMAT,
                     cmd, filename, check)!=2)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          if (!is_file((const char*)filename))
            {
              puts("FILE NOT FOUND");
              break;
            }

          is_valid_cmd = assemble_file (filename);

          break;

        case CMD_TYPE:
          // Processing input string
          snprintf((char *) __CMD_FORMAT,
                   __CMD_FORMAT_SIZE,
                   "%%%ds %%%ds %%1s",
                   __CMD_SIZE - 1,
                   __FILENAME_SIZE - 1);
          if (sscanf(input,
                     (const char *) __CMD_FORMAT,
                     cmd, filename, check)!=2)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          if (!is_file((const char*)filename))
            {
              puts("FILE NOT FOUND");
              break;
            }
          else
            {
              print_file((const char*)filename);
              is_valid_cmd = true;
            }

          break;

        case CMD_SYMBOL:
          if(sscanf(input, "%*s %1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }

          print_symbol_table ();
          is_valid_cmd = true;

          break;

        case CMD_PROGADDR:
          if(sscanf(input, "%*s %*x %1s", check) == 1
             || sscanf(input, "%*s %x", &i) != 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
          if (i < 0 || i >= __MEMORY_SIZE)
            {
              puts("INVALID PROGRAM ADDRESS");
              break;
            }

          set_progaddr ((uint32_t) i);
          is_valid_cmd = true;

          break;

        case CMD_LOADER:
          // TODO
          is_valid_cmd = true;

          break;

        case CMD_RUN:
          // TODO
          is_valid_cmd = true;

          break;

        case CMD_BP:
          // TODO
          is_valid_cmd = true;

          break;

        default:
          if(sscanf(input, "%1s", check) == 1)
            {
              puts("WRONG INSTRUCTION");
              break;
            }
        }

      if (is_valid_cmd)
        {
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
        } 
    }


memory_clear:
  if (mem != NULL)
    free (mem);
  if (input != NULL)
    free (input);
  if (cmd != NULL)
    free (cmd);
  while (!q_empty(&cmd_queue))
    {
      struct q_elem *e = q_delete(&cmd_queue);
      struct cmd_elem *ce = q_entry(e, struct cmd_elem, elem);
      if (ce->cmd != NULL)
        free(ce->cmd);
      free(ce);
    }
  free_oplist ();

  return 0;
}
