// thread.h

#ifndef _THREAD_H
#define _THREAD_H

typedef struct Ticket_mutex {
  volatile u64 ticket;
  volatile u64 serving;
} Ticket_mutex;

extern void spin_wait(void);
extern u32 thread_get_id(void);
extern u64 atomic_fetch_add(volatile u64* target, u64 value);
extern u64 atomic_compare_exchange(volatile u64* target, u64 value, u64 expected);
extern void ticket_mutex_begin(Ticket_mutex* mutex);
extern void ticket_mutex_end(Ticket_mutex* mutex);

#endif // _THREAD_H
