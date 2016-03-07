#include "hexctrl.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

static inline bool
is_inside (int val, int min, int max)
{
  return val >= min && val <= max;
}

static inline bool
is_printable (char c)
{
  return is_inside ((int) c, 0x20, 0x7E);
}

static void
hexdump (void *mem, int start, int finish)
{
  uint8_t *umem = (uint8_t *) mem;
  int i,j;
  
  for(i=start/0x10; i<=finish/0x10; ++i)
    {
      // First index numbers
      printf("%05x ", i*0x10);

      // Second hex numbers
      // One-liner
      if (i==start/0x10 && i==finish/0x10)
        {
          for(j=0; j<start%0x10; ++j)
            printf("   ");
          for(; j<=finish%0x10; ++j)
            printf("%2X ", umem[i*0x10 + j]);
          for(; j<0x10; ++j)
            printf("   ");
        }
      // Starters
      else if (i==start/0x10)
        {
          for(j=0; j<start%0x10; ++j)
            printf("   ");
          for(; j<0x10; ++j)
            printf("%2X ", umem[i*0x10 + j]);
        }
      // Finishers
      else if (i==finish/0x10)
        {
          for(j=0; j<=finish%0x10; ++j)
            printf("%2X ", umem[i*0x10 + j]);
          for(; j<0x10; ++j)
            printf("   ");
        }
      // Full-liner
      else
        {
          for(j=0; j<0x10; ++j)
            printf("%2X ", umem[i*0x10 + j]);
        }

      printf("; ");

      // Last ascii chars
      for(j=0; j<0x10; ++j)
        {
          if (is_inside(i*0x10 + j, start, finish)
              && is_printable(umem[i*0x10 + j]))
            putchar((char)umem[i*0x10 + j]);
          else
            putchar('.');
        }
      puts("");
    }
}

int
get_location (int c, bool update)
{
  static int curr = 0;
  if (update)
    curr = c;
  return curr;
}

void
autodump (void *mem, int size, int len)
{
  int curr = get_location(0, false);

  if (len <= 0 || curr + len - 1 >= size)
    {
      puts("OUT OF MEMORY BOUNDS.");
      return;
    }
  else
    {
      hexdump (mem, curr, curr + len - 1);
      get_location(curr + len, true);
    }
}
