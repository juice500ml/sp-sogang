#include "objctrl.h"

static uint32_t progaddr = 0x0;

void
set_progaddr (uint32_t addr)
{
  progaddr = addr;
}

uint32_t
get_progaddr (void)
{
  return progaddr;
}
