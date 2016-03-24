#ifndef __20141589_H__
#define __20141589_H__

#include "queue.h"

#define __OPCODE_FORMAT_SIZE 8

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
  CMD_ASSEMBLE = 10,
  CMD_TYPE = 11,
  CMD_SYMBOL = 12,
};

struct cmd_elem
{
  char *cmd;
  struct q_elem elem;
};

struct op_elem
{
  uint8_t code;
  char format[__OPCODE_FORMAT_SIZE];
  char *opcode;
  struct q_elem elem;
};

#endif
