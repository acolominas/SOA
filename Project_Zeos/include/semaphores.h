#include <list.h>
#include <types.h>

int sem_init (int n_sem, unsigned int value);

int sem_wait (int n_sem);

int sem_signal (int n_sem);

int sem_destroy (int n_sem);

int get_free_sem();

int free_sem(int n_sem);

int get_sem_id(struct list_head *sem);

int is_sem_empty();

int are_2_free_sem();

int unblock_waiters(int n_sem);
