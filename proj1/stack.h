#ifndef __STACK_H__
#define __STACK_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct s_elem 
{
  struct s_elem *next;
};

struct stack 
{
  struct s_elem head;
};

#define s_entry(S_ELEM, STRUCT, MEMBER)           \
  ((STRUCT *) ((uint8_t *) &(S_ELEM)->next     \
               - offsetof (STRUCT, MEMBER.next)))

void s_init (struct stack *);
struct s_elem *s_begin (struct stack *);
struct s_elem *s_next (struct s_elem *);
void s_push (struct stack *, struct s_elem *);
struct s_elem *s_pop (struct stack *);
bool s_empty (struct stack *);

#endif
