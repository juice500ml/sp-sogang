#include "queue.h"

#include <stddef.h>

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
