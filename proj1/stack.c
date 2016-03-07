#include "stack.h"

#include <stddef.h>

void
s_init (struct stack *s)
{
  s->head.next = NULL;
}

struct s_elem *
s_begin (struct stack *s)
{
  return s->head.next;
}

struct s_elem *
s_next (struct s_elem *elem)
{
    return elem->next;
}

void
s_push (struct stack *s, struct s_elem *e)
{
  e->next = s->head.next;
  s->head.next = e;
}

struct s_elem *
s_pop (struct stack *s)
{
  struct s_elem *e = s->head.next;
  if (e != NULL)
    s->head.next = e->next;
  return e;
}

bool
s_empty (struct stack *s)
{
  return s->head.next == NULL;
}
