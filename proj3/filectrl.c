#include "filectrl.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// MACRO FUNCTIONS
#define _SAME_STR(s1,s2) (strcmp(s1,s2)==0)

#define __REGISTER_TABLE_SIZE 10
#define __TYPE2_EXCEPTION_TABLE_SIZE 3
#define __TABLE_SIZE 20

static const int __READ_SIZE = 128;
static struct queue *oplist;
static struct queue *symbol_table;
static const char *__REGISTER_TABLE[__REGISTER_TABLE_SIZE] = {
    "A", "X", "L", "B", "S", "T", "F", "", "PC", "SW"
};
static const char *
__TYPE2_EXCEPTION_TABLE[__TYPE2_EXCEPTION_TABLE_SIZE] = {
    "CLEAR", "SVN", "TIXR"
};

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
find_register (char *r)
{
  if (strlen (r) == 0)
    return -1;

  int i = 0;
  for (i = 0; i < __REGISTER_TABLE_SIZE; ++i)
    if (_SAME_STR (r, __REGISTER_TABLE[i]))
      return i;
  return -1;
}

static bool
find_type2_exception (char *r)
{
  int i = 0;
  for (i = 0; i < __TYPE2_EXCEPTION_TABLE_SIZE; ++i)
    if (_SAME_STR (r, __TYPE2_EXCEPTION_TABLE[i]))
      return true;
  return false;
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
              else
                puts("[OPLIST] OPLIST IS CORRUPTED");
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
  if (label == NULL)
    return -1;
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

// generate string and input in queue
static bool
insert_mid_str (int linenum, uint32_t locctr,
                char *s, struct queue *q, bool loc_print)
{
  struct str_elem *me = malloc(sizeof(struct str_elem));
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

  return true;
}

// input string in queue
static bool
insert_mod_str (char *s, struct queue *q)
{
  struct str_elem *me;

  me = malloc(sizeof(struct str_elem));
  if (me == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      return false;
    }

  me->line = malloc(sizeof(char) * (strlen (s) + 1));
  if (me->line == NULL)
    {
      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
      return false;
    }

  strcpy (me->line, s);
  q_insert (q, &(me->elem));

  return true;
}

// output last length of str_queue.
static int
last_str_len (struct queue *q)
{
  char *lastline = q_entry(q_end(q)->prev,
                           struct str_elem,
                           elem)->line;
  return strlen(lastline);
}

// input string in queue, if append is true, append str to tail
static bool
insert_fin_str (char *s, struct queue *q, bool append)
{
  struct str_elem *me;
  
  if (append)
    {
      if (q_empty (q))
        {
          puts("[ASSEMBLER] CANNOT APPEND TO NOTHING");
          return false;
        }
      me = q_entry(q_end (q)->prev, struct str_elem, elem);
      char *tmp = realloc (me->line,
                           strlen (me->line) + strlen (s) + 1);
      if (tmp == NULL)
        {
          puts("[ASSEMBLER] MEMORY INSUFFICIENT");
          return false;
        }

      me->line = tmp;
      strcat (me->line, s);
    }
  else
    {
      me = malloc(sizeof(struct str_elem));
      if (me == NULL)
        {
          puts("[ASSEMBLER] MEMORY INSUFFICIENT");
          return false;
        }

      me->line = malloc(sizeof(char) * (strlen (s) + 1));
      if (me->line == NULL)
        {
          puts("[ASSEMBLER] MEMORY INSUFFICIENT");
          return false;
        }
      
      strcpy (me->line, s);
      q_insert (q, &(me->elem));
    }

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
  
  return true;
}

static bool
is_append_needed (struct queue *q, int added)
{
  char *lastline = q_entry(q_end(q)->prev,
                           struct str_elem,
                           elem)->line;
  return (strlen(lastline) + added) <= 69;
}

bool
assemble_file (const char *filename)
{
  FILE *fp = fopen(filename, "r");
  bool asm_warning = false;
  char *input = NULL;
  char *cmd = NULL;
  char *s = NULL;
  char *base_sym = NULL;
  char chk[2];
  uint32_t locctr = 0;
  uint32_t startaddr = 0;
  int baseaddr = 0;
  int linenum = 0;
  int i = 0;
  struct q_elem *e = NULL;
  struct queue lst_queue;
  struct queue fin_queue;
  struct queue mod_queue;
  struct queue *tmp_symtbl;

  q_init (&lst_queue);
  q_init (&fin_queue);
  q_init (&mod_queue);
  tmp_symtbl = malloc (sizeof (struct queue) * __TABLE_SIZE);
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
                               &lst_queue,
                               false))
            goto end_assemble;

          // no need to locctr += 0x3. this is START.
        }
      else
        {
          printf("[ASSEMBLER] SYNTAX ERROR: [%s]\n", input);
          printf("[ASSEMBLER] WARNING: SET DEFAULT START ");
          printf("ADDR TO 0x0000\n");
          asm_warning = true;
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
                               &lst_queue,
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
          asm_warning = true;
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
                               &lst_queue,
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
                               &lst_queue,
                               false))
            goto end_assemble;

          // no need to locctr += 0x3. this is directive.
          if (_SAME_STR (cmd, "END"))
            break;
          else
            {
              if (sscanf (input, "%*s %s", cmd) != 1)
                {
                  printf("[ASSEMBLER] BASE SYMBOL NOT WRITTEN");
                  printf(" AT %d:[%s]\n", linenum, input);
                  asm_warning = true;
                }
              else
                {
                  base_sym = malloc (sizeof (char)
                                     * (strlen (cmd) + 1));
                  if (base_sym == NULL)
                    {
                      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
                      goto end_assemble;
                    }
                  strcpy (base_sym, cmd);
                }
              continue;
            }
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
                               &lst_queue,
                               true))
            goto end_assemble;
          if (!insert_symbol (cmd, locctr, tmp_symtbl))
            goto end_assemble;

          // no need to locctr += 0x3. this is directive.
          if (_SAME_STR (cmd, "END"))
            break;
          else
            {
              if (sscanf (input, "%*s %*s %s", cmd) != 1)
                {
                  printf("[ASSEMBLER] BASE SYMBOL NOT WRITTEN");
                  printf(" AT %d:[%s]\n", linenum, input);
                  asm_warning = true;
                }
              else
                {
                  base_sym = malloc (sizeof (char)
                                     * (strlen (cmd) + 1));
                  if (base_sym == NULL)
                    {
                      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
                      goto end_assemble;
                    }
                  strcpy (base_sym, cmd);
                }
              continue;
            }
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
              asm_warning = true;
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
              asm_warning = true;
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
              asm_warning = true;
              continue;
            }
        }

      // locctr delta value
      int delta = 0x3;

      // now cmd has only code. find delta
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
              asm_warning = true;
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
              asm_warning = true;
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
              asm_warning = true;
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
              asm_warning = true;
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
                  asm_warning = true;
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
                  asm_warning = true;
                  continue;
                }
            }
          else
            {
              printf("[ASSEMBLER] INVALID SYNTAX");
              printf(" AT %d:[%s]", linenum, input);
              printf(" ONLY C OR X CAN BE USED.\n");
              puts("[ASSEMBLER] WARNING: LINE IGNORED.");
              asm_warning = true;
              continue;
            }
        }

      linenum += 5;
      if (!insert_mid_str (linenum,
                           locctr,
                           input,
                           &lst_queue,
                           true))
        goto end_assemble;
      locctr += delta;

    }
  while (true);

  if (base_sym != NULL)
    baseaddr = find_symbol (base_sym, tmp_symtbl);
  if (baseaddr == -1)
    {
      printf ("[ASSEMBLER] BASE SYMBOL [%s]", base_sym);
      puts (" NOT FOUND.");
      goto end_assemble;
    }

  // PASS 2
  
  // Header Record
  if (!insert_fin_str ((char*)"H", &fin_queue, false))
    goto end_assemble;

  e = q_begin (&lst_queue);
  s = q_entry(e, struct str_elem, elem)->line;
  if (sscanf (s + 10, "%s", cmd) == 1
      && _SAME_STR(cmd, "START"))
    {
      sprintf (input, "      %06X%0X",
               startaddr, locctr - startaddr);

      e = q_next (e);
    }
  else if (sscanf (s + 10, "%*s %s", cmd) == 1
           && _SAME_STR(cmd, "START"))
    {
      sscanf (s + 10, "%s", cmd);
      sprintf (input, "%-6s%06X%06X",
               cmd, startaddr, locctr - startaddr);
      e = q_next (e);
    }
  else
    {
      // TODO: where did the error go?
      sprintf (input, "      %06X%0X",
               startaddr, locctr - startaddr);
    }
  if (!insert_fin_str (input, &fin_queue, true))
    goto end_assemble;
  
  // Text Record
  if (!insert_fin_str ((char*)"", &fin_queue, false))
    goto end_assemble;
  
  for (; e != q_end (&lst_queue); e = q_next (e))
    {
      int symbol = -1;
      int code = -1;
      bool is_type_four = false;
      int line = 0;
      int pc = -1;
      
      struct q_elem *tmp = q_next(e);
      while (tmp != q_end (&lst_queue))
        {
          s = q_entry(tmp, struct str_elem, elem)->line;
          char c = s[10];
          s[10] = '\0';
          if (sscanf (s, "%*d %X", &pc) == 1)
            {
              s[10] = c;
              break;
            }
          s[10] = c;
          tmp = q_next(tmp);
        }
      if (pc == -1)
        pc = locctr;
      s = q_entry(e, struct str_elem, elem)->line;

      // comments
      if (s[0] == '.')
        continue;

      sscanf (s, "%d", &line);
      sscanf (s + 10, "%s", cmd);
      if ((symbol = find_symbol (cmd, tmp_symtbl)) != -1)
        sscanf (s + 10, "%*s %s", cmd);

      if (cmd[0] == '+')
        {
          is_type_four = true;
          code = find_oplist (&cmd[1]);
        }
      else
        {
          code = find_oplist (cmd);
        }
      
      // it is opcode
      if (code != -1)
        {
          int type = 0;
          if (is_type_four)
            type = find_size_oplist (&cmd[1]);
          else
            type = find_size_oplist (cmd);
          if (type == 0)
            {
              puts("[ASSEMBLER][DEBUG] find_size_oplist failed");
              asm_warning = true;
              continue;
            }
          if (is_type_four)
            type = 4;
          if (type == 1)
            {
              if (symbol == -1)
                {
                  if (sscanf(s+10, "%*s %1s", chk) == 1)
                    {
                      printf ("[ASSEMBLER] TRAILING CHARACTERS");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  else
                    {
                      sprintf (cmd, "%02X", code);
                    }
                }
              else
                {
                  if (sscanf(s+10, "%*s %*s %1s", chk) == 1)
                    {
                      printf ("[ASSEMBLER] TRAILING CHARACTERS");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  else
                    {
                      sprintf (cmd, "%02X", code);
                    }
                }
            }
          else if (type == 2)
            {
              int comma_loc = 0;
              int r1 = 0, r2 = 0;

              if (find_type2_exception(cmd))
                {
                  if (symbol == -1)
                    {
                      if (sscanf (s+10, "%*s %s", input) != 1)
                        {
                          printf ("[ASSEMBLER] NO OPERAND FOUND");
                          printf (" AT %d:[%s]\n", line, s);
                          asm_warning = true;
                          continue;
                        }
                      r1 = find_register (input);
                      if (r1 == -1)
                        {
                          printf ("[ASSEMBLER] NO SUCH REGISTER");
                          printf (" AT %d:[%s]\n", line, s);
                          asm_warning = true;
                          continue;
                        }
                    }
                  else
                    {
                      if (sscanf (s+10, "%*s %*s %s", input) != 1)
                        {
                          printf ("[ASSEMBLER] NO OPERAND FOUND");
                          printf (" AT %d:[%s]\n", line, s);
                          asm_warning = true;
                          continue;
                        }
                      r1 = find_register (input);
                      if (r1 == -1)
                        {
                          printf ("[ASSEMBLER] NO SUCH REGISTER");
                          printf (" AT %d:[%s]\n", line, s);
                          asm_warning = true;
                          continue;
                        }
                    }
                  sprintf (cmd, "%02X%01X%01X", code, r1, 0);
                }
              else
                {
                  for (i = 0; s[i] != '\0'; ++i)
                    if (s[i] == ',')
                      break;
                  if (s[i] != ',')
                    {
                      printf ("[ASSEMBLER] NO COMMA SEPERATOR");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  comma_loc = i;
                  for (i = comma_loc + 1; s[i] != '\0'; ++i)
                    if (s[i] == ',')
                      break;
                  if (s[i] == ',')
                    {
                      printf ("[ASSEMBLER] MULTIPLE COMMAS");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  s[comma_loc] = ' ';
                }
              if (comma_loc != 0 && symbol == -1)
                {
                  if (sscanf (s+10, "%*s %s", input) != 1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO OPERAND FOUND");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  r1 = find_register (input);
                  if (r1 == -1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO SUCH REGISTER");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  if (sscanf (s+10, "%*s %*s %s", input) != 1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO OPERAND FOUND");
                      printf (" AT %d`:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  r2 = find_register (input);
                  if (r2 == -1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO SUCH REGISTER");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  s[comma_loc] = ',';
                  sprintf (cmd, "%02X%01X%01X", code, r1, r2);
                }
              else if (comma_loc != 0 && symbol != -1)
                {
                  if (sscanf (s+10, "%*s %*s %s", input) != 1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO OPERAND FOUND");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  r1 = find_register (input);
                  if (r1 == -1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO SUCH REGISTER");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  if (sscanf (s+10, "%*s %*s %*s %s", input) != 1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO OPERAND FOUND");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  r2 = find_register (input);
                  if (r2 == -1)
                    {
                      s[comma_loc] = ',';
                      printf ("[ASSEMBLER] NO SUCH REGISTER");
                      printf (" AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  s[comma_loc] = ',';
                  sprintf (cmd, "%02X%01X%01X", code, r1, r2);
                }
            }
          else if (type == 3 || type == 4)
            {
              int comma_loc = -1;
              int xbpe = 0;

              for (i=10; s[i]!='\0'; ++i)
                if (s[i] == ',')
                  break;
              if (s[i] == ',')
                {
                  comma_loc = i;
                  for (i=i+1; s[i]!='\0'; ++i)
                    {
                      if (s[i] == ' ' || s[i] == '\t')
                        continue;
                      else if(s[i] == 'X')
                        break;
                    }
                  if (s[i] != 'X')
                    {
                      printf ("[ASSEMBLER] NO OPERAND AFTER");
                      printf ("COMMA AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  for (i=i+1; s[i]!='\0'; ++i)
                    if (s[i] != ' ' && s[i] != '\t')
                      break;
                  if (s[i] != '\0')
                    {
                      printf ("[ASSEMBLER] TRAILING CHARACTERS");
                      printf ("AT %d:[%s]\n", line, s);
                      asm_warning = true;
                      continue;
                    }
                  // x on
                  xbpe = 8;
                }

              input[0] = '\0';
              if (comma_loc != -1)
                s[comma_loc] = '\0';
              if (symbol == -1)
                sscanf (s + 10, "%*s %s", input);
              else
                sscanf (s + 10, "%*s %*s %s", input);
              if (comma_loc != -1)
                s[comma_loc] = ',';
              if (strlen(input) == 0 && !_SAME_STR("RSUB", cmd))
                {
                  printf ("[ASSEMBLER] NO OPERAND");
                  printf (" AT %d:[%s]\n", line, s);
                  asm_warning = true;
                  continue;
                }

              if (_SAME_STR ("RSUB", cmd))
                {
                  sprintf (cmd, "%02X0000", code);
                }
              // immediate addressing
              else if (input[0] == '#')
                {
                  // n=0, i=1
                  code += 1;
                  int disp = find_symbol (&input[1], tmp_symtbl);
                  if (disp == -1)
                    {
                      if (sscanf(&input[1],
                                 "%d %s",
                                 &disp,
                                 chk) != 1)
                        {
                          printf ("[ASSEMBLER] SYMBOL NOT FOUND");
                          printf (" AT %d:[%s]\n", line, s);
                          asm_warning = true;
                          continue;
                        }
                      if (disp > 4095)
                        {
                          // e=1
                          if (!is_type_four)
                            {
                              printf("[ASSEMBLER] OPERAND TOO ");
                              printf("BIG AT %d:[%s]\n", line, s);
                              asm_warning = true;
                              continue;
                            }
                          xbpe += 1;
                        }
                    }
                  else
                    {
                      if ((disp-pc) >= -2048 && (disp-pc) <= 2047)
                        {
                          // b=0, p=1
                          xbpe += 2;
                          disp -= pc;
                        }
                      else if ( base_sym!=NULL && (disp-baseaddr) >= 0
                               && (disp-baseaddr) <= 4095)
                        {
                          // b=1, p=0
                          xbpe += 4;
                          disp -= baseaddr;
                        }
                      else
                        {
                          // e=1
                          if (!is_type_four)
                            {
                              printf("[ASSEMBLER] ADDRESS TOO ");
                              printf("BIG AT %d:[%s]\n", line, s);
                              asm_warning = true;
                              continue;
                            }
                          xbpe += 1;
                        }
                    }

                  if (is_type_four)
                    sprintf (cmd, "%02X%01X%05X",
                             code, xbpe, disp);
                  else  
                    sprintf (cmd, "%02X%01X%03X",
                             code, xbpe, disp);
                }
              else
                {
                  int disp = -1;
                  // indirect addressing
                  // n=1, i=0
                  if (input[0] == '@')
                    {
                      disp = find_symbol (&input[1], tmp_symtbl);
                      code += 2;
                    }
                  // simple addressing
                  // n=1, i=1
                  else
                    {
                      disp = find_symbol (input, tmp_symtbl);
                      code += 3;
                    }
                  if (disp == -1)
                    {
                      printf ("[ASSEMBLER] SYMBOL NOT FOUND");
                      printf (" AT %d:[%s]\n", line, s);
                      printf ("[ASSEMBLER] SET DEFAULT VALUE.");
                      disp = pc;
                      asm_warning = true;
                    }

                  if ((disp-pc) >= -2048 && (disp-pc) <= 2047)
                    {
                      // b=0, p=1
                      xbpe += 2;
                      disp -= pc;
                    }
                  else if ( base_sym != NULL && (disp-baseaddr) >= 0
                           && (disp-baseaddr) <= 4095)
                    {
                      // b=1, p=0
                      xbpe += 4;
                      disp -= baseaddr;
                    }
                  else
                    {
                      // e=1
                      if (!is_type_four)
                        {
                          printf ("[ASSEMBLER] ADDRESS TOO BIG");
                          printf (" AT %d:[%s]\n", line, s);
                          asm_warning = true;
                          continue;
                        }
                      xbpe += 1;
                    }
                  if (is_type_four)
                    {
                      sprintf (cmd, "%02X%01X%05X",
                               code, xbpe, disp);
                      sscanf (s, "%*d %X", &i);
                      sprintf (input, "M%06X05", i-startaddr+1);
                      insert_mod_str (input, &mod_queue);
                    }
                  else
                    {
                      if (disp >= 0)
                        sprintf (cmd, "%02X%01X%03X",
                                 code, xbpe, disp);
                      else
                        {
                          sprintf (input, "%X", disp);
                          i = strlen (input) - 3;
                          sprintf (cmd, "%02X%01X%s",
                                   code, xbpe, &input[i]);
                        }
                    }
                  // TODO if it is type four, mod record!
                }
            }
          else
            {
              puts("[ASSEMBLER][DEBUG] type finding failed");
              continue;
            }

          int lastlen = last_str_len (&fin_queue);
          bool is_append = is_append_needed (&fin_queue,
                                             strlen (cmd));
          if (!is_append || lastlen == 0)
            {
              sscanf (s, "%*d %X", &i);
              sprintf (input, "T%06X--", i);
              if (!insert_fin_str (input,
                                   &fin_queue,
                                   lastlen == 0))
                goto end_assemble;
            }
          if (!insert_fin_str (cmd, &fin_queue, true))
            goto end_assemble;

          char *tmp = realloc (s, strlen (s) + strlen (cmd) + 3);
          if (tmp == NULL)
            {
              puts("[ASSEMBLER] MEMORY INSUFFICIENT");
              goto end_assemble;
            }
          q_entry(e, struct str_elem, elem)->line = tmp;
          s = tmp;
          strcat (s, "\t\t");
          strcat (s, cmd);
        }
      // it is directive
      else
        {
          if (_SAME_STR(cmd, "END"))
            break;
          else if (_SAME_STR(cmd, "BASE"))
            continue;
          else if (_SAME_STR(cmd, "RESB"))
            {
              int lastlen = last_str_len (&fin_queue);
              if (!insert_fin_str ((char*)"",
                                   &fin_queue,
                                   lastlen == 0))
                goto end_assemble;
            }
          else if (_SAME_STR(cmd, "RESW"))
            {
              int lastlen = last_str_len (&fin_queue);
              if (!insert_fin_str ((char*)"",
                                   &fin_queue,
                                   lastlen == 0))
                goto end_assemble;
            }
          else if (_SAME_STR(cmd, "BYTE"))
            {
              int apo_loc = 0;
              int totlen = strlen(s);

              for (i=0; s[i]!='\0'; ++i)
                if (s[i] == '\'')
                  break;
              apo_loc = i + 1;
              for (i = apo_loc; s[i]!='\0'; ++i)
                if (s[i] == '\'')
                  break;
              if (s[apo_loc-2]=='C')
                {
                  s[i] = '\0';
                  char *tmp = &s[apo_loc];
                  int tmplen = strlen (tmp);
                  s[i] = '\'';
                  bool is_append = is_append_needed (&fin_queue,
                                                     tmplen);
                  char *new = realloc (s,
                                  totlen + strlen (tmp) * 2 + 4);
                  if (new == NULL)
                    {
                      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
                      goto end_assemble;
                    }
                  q_entry(e, struct str_elem, elem)->line = new;
                  s = new;
                  strcat (s, "\t\t");
                  
                  int j = apo_loc;
                  for (j = apo_loc; j < tmplen + apo_loc; ++j)
                    {
                      sprintf(input, "%02X", (int) s[j]);
                      if (!insert_fin_str (input,
                                           &fin_queue,
                                           is_append))
                        goto end_assemble;
                      strcat (s, input);
                    }
                }
              else
                {
                  int num;
                  sscanf (&s[apo_loc], "%X", &num);
                  sprintf(cmd, "%X", num);
                  if (strlen(cmd) % 2 == 1)
                    sprintf(cmd, "0%X", num);
                  bool is_append = is_append_needed (&fin_queue,
                                                     strlen(cmd));
                  if (!insert_fin_str(cmd, &fin_queue, is_append))
                    goto end_assemble;

                  char *tmp = realloc (s, totlen
                                          + strlen(cmd) + 3);
                  if (tmp == NULL)
                    {
                      puts("[ASSEMBLER] MEMORY INSUFFICIENT");
                      goto end_assemble;
                    }
                  q_entry(e, struct str_elem, elem)->line = tmp;
                  s = tmp;
                  strcat (s, "\t\t");
                  strcat (s, cmd);
                }
            }
          else if (_SAME_STR(cmd, "WORD"))
            {
              int num = 0;
              if (sscanf (&s[10], "%*s %d", &num) != 1)
                sscanf (&s[10], "%*s %*s %d", &num);
              sprintf(cmd, "%X", num);
              bool is_append = is_append_needed (&fin_queue,
                                                 strlen(cmd));
              if (!insert_fin_str(cmd, &fin_queue, is_append))
                goto end_assemble;
              
              char *tmp = realloc (s, strlen(s)
                                      + strlen(cmd) + 3);
              if (tmp == NULL)
                {
                  puts("[ASSEMBLER] MEMORY INSUFFICIENT");
                  goto end_assemble;
                }
              q_entry(e, struct str_elem, elem)->line = tmp;
              s = tmp;
              strcat (s, "\t\t");
              strcat (s, cmd);
            }
        }
    }

  // Mod Record & length of Text Record
  for (e=q_begin(&fin_queue); e!=q_end(&fin_queue); e=q_next(e))
    {
      s = q_entry (e, struct str_elem, elem)->line;
      if (s[0] == 'T')
        {
          i = strlen(s)-9;
          sprintf (input, "%02X", i/2);
          s[7] = input[0];
          s[8] = input[1];
        }
    }

  // no warning generated.
  if (!asm_warning)
    {
      printf("output file : ");
      sprintf (cmd, "%s", filename);
      for (i=strlen(cmd)-1; i>=0; --i)
        if (cmd[i] == '.')
          break;
      
      sprintf (&cmd[i+1], "lst");
      FILE *new_fp = fopen ((const char*) cmd, "w+");
      for (e = q_begin (&lst_queue);
           e != q_end (&lst_queue);
           e = q_next (e))
        {
          s = q_entry (e, struct str_elem, elem)->line;
          fprintf (new_fp, "%s\n", s);
        }
      fclose (new_fp);
      printf("[%s], ", cmd);

      sprintf (&cmd[i+1], "obj");
      new_fp = fopen ((const char*) cmd, "w+");
      for (e = q_begin (&fin_queue);
           e != q_end (&fin_queue);
           e = q_next (e))
        {
          s = q_entry (e, struct str_elem, elem)->line;
          fprintf (new_fp, "%s\n", s);
        }
      for (e = q_begin (&mod_queue);
           e != q_end (&mod_queue);
           e = q_next (e))
        {
          s = q_entry (e, struct str_elem, elem)->line;
          fprintf (new_fp, "%s\n", s);
        }
      sprintf (input, "E%06X", startaddr);
      fprintf (new_fp, "%s\n", input);
      fclose (new_fp);
      printf("[%s]\n", cmd);
    }


end_assemble:
  if (fp != NULL)
    fclose (fp);
  if (input != NULL)
    free (input);
  if (cmd != NULL)
    free (cmd);
  if (base_sym != NULL)
    free (base_sym);

  while (!q_empty(&lst_queue))
    {
      struct q_elem *e = q_delete (&lst_queue);
      struct str_elem *me
        = q_entry (e, struct str_elem, elem);
      if (me->line != NULL)
        free (me->line);
      free (me);
    }
  while (!q_empty(&fin_queue))
    {
      struct q_elem *e = q_delete (&fin_queue);
      struct str_elem *me
        = q_entry (e, struct str_elem, elem);
      if (me->line != NULL)
        free (me->line);
      free (me);
    }
  while (!q_empty(&mod_queue))
    {
      struct q_elem *e = q_delete (&mod_queue);
      struct str_elem *me
        = q_entry (e, struct str_elem, elem);
      if (me->line != NULL)
        free (me->line);
      free (me);
    }
  if (asm_warning)
    {
      if (tmp_symtbl != NULL)
        {
          for (i = 0; i < __TABLE_SIZE; ++i)
            {
              while (!q_empty(&(tmp_symtbl[i])))
                {
                  struct q_elem *e = q_delete (&tmp_symtbl[i]);
                  struct sym_elem *se
                    = q_entry (e, struct sym_elem, elem);
                  if (se->label != NULL)
                    free (se->label);
                  free (se);
                }
            }
          free (tmp_symtbl);
        }
      if (symbol_table != NULL)
        for (i = 0; i < __TABLE_SIZE; ++i)
          {
            while (!q_empty(&(symbol_table[i])))
              {
                struct q_elem *e = q_delete (&symbol_table[i]);
                struct sym_elem *se
                  = q_entry (e, struct sym_elem, elem);
                if (se->label != NULL)
                  free (se->label);
                free (se);
              }
          }
      symbol_table = NULL;
    }
  else
    symbol_table = tmp_symtbl;

  if (asm_warning)
    puts ("assemble failed.");
  
  return !asm_warning;
}

void
print_symbol_table (void)
{
  if (symbol_table == NULL)
    return;
  unsigned int i, j = 0, totelem = 0;
  for (i = 0; i < __TABLE_SIZE; ++i)
    totelem += q_size(&symbol_table[i]);
  struct sym_elem **se = malloc (sizeof(struct sym_elem *)
                                 * totelem);
  for (i = 0; i < __TABLE_SIZE; ++i)
    {
      struct q_elem *e = q_begin (&symbol_table[i]);
      for(; e != q_end (&symbol_table[i]); e = q_next (e))
        se[j++] = q_entry (e, struct sym_elem, elem);
    }
  for (i = 0; i < totelem; ++i)
    for (j = i + 1; j < totelem; ++j)
      if ( strcmp(se[i]->label, se[j]->label) > 0)
        {
          struct sym_elem *tmp = se[i];
          se[i] = se[j];
          se[j] = tmp;
        }
  for (i = 0; i < totelem; ++i)
    printf ("\t%s\t%04X\n", se[i]->label, se[i]->locctr);
  free (se);
}
