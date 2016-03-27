#include "queue.h"

#include <stddef.h>

// Hash function for char string
uint64_t
str_hash (char *str)
{
  // hash start value is prime number
  uint64_t c, hash = 179426549;

  while ((c = *str++)!='\0')
    hash = ((hash << 2) + hash) + c;

  return hash;
}

void
q_init (struct queue *q)
{
  q->head.prev = NULL;
  q->head.next = &q->tail;
  q->tail.prev = &q->head;
  q->tail.next = NULL;
}

struct q_elem *
q_begin (struct queue *q)
{
  return q->head.next;
}

struct q_elem *
q_end (struct queue *q)
{
  return &q->tail;
}

struct q_elem *
q_next (struct q_elem *elem)
{
    return elem->next;
}

void
q_insert (struct queue *q, struct q_elem *e)
{
  struct q_elem *b = q_end (q);
  e->prev = b->prev;
  e->next = b;
  b->prev->next = e;
  b->prev = e;
}

struct q_elem *
q_delete (struct queue *q)
{
  struct q_elem *e = q_begin (q);
  e->prev->next = e->next;
  e->next->prev = e->prev;
  return e;
}

bool
q_empty (struct queue *q)
{
  return q_begin(q) == q_end(q);
}

unsigned int
q_size (struct queue *q)
{
  unsigned int i = 0;
  struct q_elem *e = q_begin(q);
  for (; e != q_end(q); e = q_next(e))
    i++;
  return i;
}
