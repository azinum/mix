// thread.h

#ifndef _THREAD_H
#define _THREAD_H

typedef struct Ticket_mutex {
  volatile size_t ticket;
  volatile size_t serving;
} Ticket_mutex;

extern void spin_wait(void);
extern u32 thread_get_id(void);
extern size_t atomic_fetch_add(volatile size_t* target, size_t value);
extern size_t atomic_compare_exchange(volatile size_t* target, size_t value, size_t expected);
extern void ticket_mutex_begin(Ticket_mutex* mutex);
extern void ticket_mutex_end(Ticket_mutex* mutex);

#endif // _THREAD_H
