#include "hexctrl.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

static inline bool
is_inside (uint32_t val, uint32_t min, uint32_t max)
{
  return val >= min && val <= max;
}

static inline bool
is_printable (uint8_t c)
{
  return is_inside (c, 0x20, 0x7E);
}

static void
hexdump (void *mem, uint32_t start, uint32_t finish)
{
  uint8_t *umem = (uint8_t *) mem;
  uint32_t i,j;
  
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
            printf("%02X ", umem[i*0x10 + j]);
          for(; j<0x10; ++j)
            printf("   ");
        }
      // Starters
      else if (i==start/0x10)
        {
          for(j=0; j<start%0x10; ++j)
            printf("   ");
          for(; j<0x10; ++j)
            printf("%02X ", umem[i*0x10 + j]);
        }
      // Finishers
      else if (i==finish/0x10)
        {
          for(j=0; j<=finish%0x10; ++j)
            printf("%02X ", umem[i*0x10 + j]);
          for(; j<0x10; ++j)
            printf("   ");
        }
      // Full-liner
      else
        {
          for(j=0; j<0x10; ++j)
            printf("%02X ", umem[i*0x10 + j]);
        }

      printf("; ");

      // Last ascii chars
      for(j=0; j<0x10; ++j)
        {
          if (is_inside(i*0x10 + j, start, finish)
              && is_printable(umem[i*0x10 + j]))
            putchar(umem[i*0x10 + j]);
          else
            putchar('.');
        }
      puts("");
    }
}

uint32_t
get_location (uint32_t c, bool update)
{
  static uint32_t curr = 0;
  if (update)
    curr = c;
  return curr;
}

void
autodump (void *mem, uint32_t size, uint32_t len)
{
  uint32_t curr = get_location(0, false);

  if (curr >= size || curr + len - 1 >= size)
    {
      puts("OUT OF MEMORY BOUNDS.");
    }
  else
    {
      hexdump (mem, curr, curr + len - 1);
      get_location(curr + len, true);
    }
}

void
hexfill (uint8_t *mem, uint32_t size,
         uint32_t start, uint32_t end, uint8_t value)
{
  if (start > end || end >= size)
    {
      puts("OUT OF MEMORY BOUNDS.");
    }
  else
    {
      uint32_t i = 0;
      for (i=start; i<=end; ++i)
        mem[i] = value;
    }
}
