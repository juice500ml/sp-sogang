#include "filectrl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
