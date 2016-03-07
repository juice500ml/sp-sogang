#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct q_elem 
{
  struct q_elem *next;
  struct q_elem *prev;
};

struct queue 
{
  struct q_elem head;
  struct q_elem tail;
};

#define q_entry(Q_ELEM, STRUCT, MEMBER)           \
  ((STRUCT *) ((uint8_t *) &(Q_ELEM)->next     \
               - offsetof (STRUCT, MEMBER.next)))

void q_init (struct queue *);
struct q_elem *q_begin (struct queue *);
struct q_elem *q_end (struct queue *);
struct q_elem *q_next (struct q_elem *);
void q_insert (struct queue *, struct q_elem *);
struct q_elem *q_delete (struct queue *q);
struct q_elem *q_pop (struct queue *);
bool q_empty (struct queue *);

#endif
