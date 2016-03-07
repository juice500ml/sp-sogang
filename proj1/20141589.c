#include <stdio.h>
#include <stdlib.h>

// MACRO FUNCTIONS
#define _ERROR_EXIT(s,i) do{\
    perror(s);\
    exit(i);\
} while(0);

// FIXED VARIABLES
const int __INPUT_SIZE = 64;
const char *__SHELL_FORM = "sicsim> ";

int
main(void)
{
  // INITIALIZING
  char *input = malloc(sizeof(char)*__INPUT_SIZE);
  if (input == NULL)
    _ERROR_EXIT("INPUT STRING MEMORY SIZE INSUFFICIENT", -1);

  return 0;
}
