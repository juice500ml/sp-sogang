#include "filectrl.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// MACRO FUNCTIONS
#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

static const int __TABLE_SIZE = 20;
static const int __READ_SIZE = 128;
static struct queue *oplist;
static struct queue *symbol_table;

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
          if (_SAME_STR(cmd, oe->opcode))
            return oe->code;
        }
    }
  return -1;
}

static int
find_size_oplist (char *cmd)
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
          if (_SAME_STR(cmd, oe->opcode))
            {
              if (_SAME_STR(oe->format, "1"))
                return 1;
              else if (_SAME_STR(oe->format, "2"))
                return 2;
              else if (_SAME_STR(oe->format, "3/4"))
                return 3;
            }
        }
    }
  return 0;
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

// if not found, return -1, if found, locctr
static int
find_symbol (char *label, struct queue tbl[])
{
  uint64_t i = str_hash (label) % __TABLE_SIZE;
  if (!q_empty(&tbl[i]))
    {
      struct q_elem *qe = q_begin (&tbl[i]);
      for (; qe != q_end (&tbl[i]); qe = q_next (qe))
        {
          struct sym_elem *se
            = q_entry (qe, struct sym_elem, elem);
          if (_SAME_STR(label, se->label))
            return se->locctr;
        }
    }
  return -1;
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
insert_mid_str (int linenum, uint32_t locctr,
                char *s, struct queue *q, bool loc_print)
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
  if (loc_print)
    sprintf (me->line, "%4d %04X %s", linenum, locctr, s);
  else  
    sprintf (me->line, "%4d      %s", linenum, s);
  q_insert (q, &(me->elem));
  printf("[%s] inserted!\n", me->line);
  return true;
}

// input symbol in queue
static bool
insert_symbol (char *label, int ctr, struct queue tbl[])
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
  struct queue tmp_symtbl[__TABLE_SIZE];

  q_init (&mid_queue);
  for (i=0; i<__TABLE_SIZE; ++i)
    q_init (&tmp_symtbl[i]);

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

  // PASS 1
  // ONLY CHECK FRONT TWO STRINGS

  // check if empty
  if (!get_str(input, __READ_SIZE, fp))
    {
      printf("[ASSEMBLER] %s IS EMPTY\n", filename);
      goto end_assemble;
    }
  
  // START
  // ex) START 1000
  if (sscanf (input, "%s", cmd) == 1
      && _SAME_STR(cmd, "START"))
    {
      if (sscanf(input, "%*s %s %1s", cmd, chk) == 1
          && sscanf(cmd, "%x %1s", &startaddr, chk) == 1)
        {
          locctr = startaddr;
          linenum += 5;

          // insertion
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue,
                               false))
            goto end_assemble;

          // no need to locctr += 0x3. this is START.
        }
      else
        {
          printf("[ASSEMBLER] SYNTAX ERROR: [%s]\n", input);
          printf("[ASSEMBLER] WARNING: SET DEFAULT START ");
          printf("ADDR TO 0x0000\n");
        }
    }
  // ex) COPY START 1000
  else if (sscanf (input, "%*s %s", cmd) == 1
           && _SAME_STR(cmd, "START"))
    {
      if (sscanf(input, "%*s %*s %s %1s", cmd, chk) == 1
          && sscanf(cmd, "%x %1s", &startaddr, chk) == 1)
        {
          locctr = startaddr;
          linenum += 5;
          sscanf(input, "%s", cmd);

          // insertion with linenumber
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue,
                               true))
            goto end_assemble;
          if (!insert_symbol (cmd, locctr, tmp_symtbl))
            goto end_assemble;

          // no need to locctr += 0x3. this is START.
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
      bool is_type_four = false;

      // read line until valid
      if (!get_str(input, __READ_SIZE, fp))
        {
          printf("[ASSEMBLER] UNEXPECTED EOF\n");
          goto end_assemble;
        }

      // check if comment
      if (input[0] == '.')
        {
          linenum += 5;

          // insertion
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue,
                               false))
            goto end_assemble;

          continue;
        }

      // check for directives: if END or BASE
      if (sscanf (input, "%s", cmd) == 1
          && (_SAME_STR(cmd, "END")||_SAME_STR(cmd, "BASE")))
        {
          linenum += 5;

          // insertion
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue,
                               false))
            goto end_assemble;

          // no need to locctr += 0x3. this is directive.
          if (_SAME_STR (cmd, "END"))
            break;
          else
            continue;
        }
      else if (sscanf (input, "%*s %s", cmd) == 1
               && (_SAME_STR(cmd, "END")||_SAME_STR(cmd, "BASE")))
        {
          linenum += 5;
          sscanf(input, "%s", cmd);

          // insertion with linenumber
          if (!insert_mid_str (linenum,
                               locctr,
                               input,
                               &mid_queue,
                               true))
            goto end_assemble;
          if (!insert_symbol (cmd, locctr, tmp_symtbl))
            goto end_assemble;

          // no need to locctr += 0x3. this is directive.
          if (_SAME_STR (cmd, "END"))
            break;
          else
            continue;
        }

      // check for others
      // read first one and determine if opcode
      if (sscanf (input, "%s", cmd) != 1)
        {
          printf("[ASSEMBLER][DEBUG] get_str behavior error\n");
          goto end_assemble;
        }
      if (cmd[0] == '+')
        {
          strcpy (cmd, &cmd[1]);
          is_type_four = true;
        }

      // it has to be a symbol
      if (find_oplist (cmd) == -1
          && !_SAME_STR (cmd, "WORD")
          && !_SAME_STR (cmd, "RESW")
          && !_SAME_STR (cmd, "RESB")
          && !_SAME_STR (cmd, "BYTE"))
        {
          is_type_four = false;
          if (find_symbol (cmd, tmp_symtbl) != -1)
            {
              printf("[ASSEMBLER] DUPLICATE SYMBOL [%s]", cmd);
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: SYMBOL IGNORED.\n");
            }
          else
            {
              if (!insert_symbol (cmd, locctr, tmp_symtbl))
                goto end_assemble;
            }
          
          // save new command at cmd
          if (sscanf (input, "%*s %s", cmd) != 1)
            {
              printf("[ASSEMBLER] INVALID SYNTAX");
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: LINE IGNORED.\n");
              continue;
            }
          if (cmd[0] == '+')
            {
              is_type_four = true;
              strcpy (cmd, &cmd[1]);
            }

          if (find_oplist (cmd) == -1
              && !_SAME_STR (cmd, "WORD")
              && !_SAME_STR (cmd, "RESW")
              && !_SAME_STR (cmd, "RESB")
              && !_SAME_STR (cmd, "BYTE"))
            {
              printf("[ASSEMBLER] NO SUCH OPCODE [%s]", cmd);
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: LINE IGNORED.\n");
              continue;
            }
        }

      // locctr delta value
      int delta = 0x3;

      // now cmd has only code
      if (is_type_four)
        delta = 0x4;
      else if (find_size_oplist (cmd) == 1)
        delta = 0x1;
      else if (find_size_oplist (cmd) == 2)
        delta = 0x2;
      else if (_SAME_STR (cmd, "RESW"))
        {
          if (sscanf (input, "%*s %*s %d", &i) != 1)
            {
              printf("[ASSEMBLER] INVALID SYNTAX");
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: LINE IGNORED.\n");
              continue;
            }
          delta = i * 0x3;
        }
      else if (_SAME_STR (cmd, "RESB"))
        {
          if (sscanf (input, "%*s %*s %d", &i) != 1)
            {
              printf("[ASSEMBLER] INVALID SYNTAX");
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: LINE IGNORED.\n");
              continue;
            }
          delta = i;
        }
      else if (_SAME_STR (cmd, "BYTE"))
        {
          if (sscanf (input, "%*s %*s %s", cmd) != 1)
            {
              printf("[ASSEMBLER] INVALID SYNTAX");
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: LINE IGNORED.\n");
              continue;
            }

          int numcolon = 0;
          int numbyte = 0;
          for(i=0; input[i]!='\0'; ++i)
            if(input[i] == '\'')
              ++numcolon;
          if (numcolon != 2)
            {
              printf("[ASSEMBLER] APOSTROPHE DO NOT MATCH");
              printf(" AT %d:[%s]\n", linenum, input);
              printf("[ASSEMBLER] WARNING: LINE IGNORED.\n");
              continue;
            }
          for(i=0; input[i]!='\0'; ++i)
            if(input[i] == '\'')
              break;

          if (input[i-1] == 'C')
            {
              ++i;
              while (input[i] != '\'')
                {
                  ++i;
                  ++numbyte;
                }
              if (input[i+1] != '\0')
                {
                  printf("[ASSEMBLER] TRAILING CHARACTERS");
                  printf(" AT %d:[%s]\n", linenum, input);
                  puts("[ASSEMBLER] WARNING: LINE IGNORED.");
                  continue;
                }
              delta = numbyte;
            }
          else if (input[i-1] == 'X')
            {
              numcolon = i;

              // check for chars inside apostrophe
              for (i=i+1; input[i]!='\''; ++i)
                if (!(input[i]>='a' && input[i]<='f')
                    && !(input[i]>='0' && input[i] <= '9')
                    && !(input[i]>='A' && input[i] <= 'F'))
                  break;

              // check for trailing chars
              if (input[i] == '\''
                  && input[i+1] == '\0'
                  && sscanf (input + numcolon + 1,
                             "%x", &numbyte) == 1 )
                {
                  sprintf(cmd, "%X", numbyte);
                  delta = (strlen (cmd) + 1) / 2;
                }
              else
                {
                  if (input[i] == '\'' && input[i+1] != '\0')
                    printf("[ASSEMBLER] TRAILING CHARACTERS");
                  else
                    printf("[ASSEMBLER] NOT A HEX VALUE");
                  printf(" AT %d:[%s]\n", linenum, input);
                  puts("[ASSEMBLER] WARNING: LINE IGNORED.");
                  continue;
                }
            }
          else
            {
              printf("[ASSEMBLER] INVALID SYNTAX");
              printf(" AT %d:[%s]", linenum, input);
              printf(" ONLY C OR X CAN BE USED.\n");
              puts("[ASSEMBLER] WARNING: LINE IGNORED.");
              continue;
            }
        }

      linenum += 5;
      if (!insert_mid_str (linenum,
                           locctr,
                           input,
                           &mid_queue,
                           true))
        goto end_assemble;
      locctr += delta;

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
      struct mid_elem *me
        = q_entry (e, struct mid_elem, elem);
      puts(me->line);
      if (me->line != NULL)
        free (me->line);
      free (me);
    }
  // else save mid file and free
  // if not assembled, free with getter/setter!
  for (i=0; i<__TABLE_SIZE; ++i)
    {
      while (!q_empty(&(tmp_symtbl[i])))
        {
          struct q_elem *e = q_delete (&(tmp_symtbl[i]));
          struct sym_elem *se
            = q_entry (e, struct sym_elem, elem);
          if (se->label != NULL)
            free (se->label);
          free (se);
        }
    }
  // else save

  return is_assembled;
}
