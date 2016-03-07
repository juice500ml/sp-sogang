#ifndef __20141589_H__
#define __20141589_H__

#include "stack.h"

enum cmd_flags
{
  CMD_HELP = 0,
  CMD_DIR = 1,
  CMD_QUIT = 2,
  CMD_HISTORY = 3,
  CMD_DUMP = 4,
  CMD_EDIT = 5,
  CMD_FILL = 6,
  CMD_RESET = 7,
  CMD_OPCODE = 8,
  CMD_OPCODELIST = 9,
};

struct cmd_elem
{
  char *cmd;
  struct s_elem elem;
};

#endif
