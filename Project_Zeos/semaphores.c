#include <semaphores.h>
#include <sched.h>
#include <stddef.h>

#define NR_SEM 10
struct sem_t sem[NR_SEM];

struct list_head freesem_queue;
extern struct list_head readyqueue;

int get_free_sem() {
   int a = -1;
   if (!list_empty(&freesem_queue)) {
    struct list_head *l = list_first(&freesem_queue);
    a = get_sem_id(l);
    list_del(l);
   }
  return a;
}

//retorna el ID dado un list_head de sem
int get_sem_id(struct list_head *sem)
{
  return *((int*)((int)sem+sizeof(struct list_head)));
}

int sem_init (int n_sem, unsigned int value) {
  if (n_sem < 0 || n_sem > NR_SEM) return -1;
  if (value < 0) return -1;
  if (sem[n_sem].count != NULL) return -1;
  sem[n_sem].count = value;
  sem[n_sem].destroyed = 0;
  INIT_LIST_HEAD(&(sem[n_sem].blocked));
  return 0;
}

int sem_wait (int n_sem) {
  if (n_sem < 0 || n_sem > NR_SEM) return -1;
  sem[n_sem].count --;
  if (sem[n_sem].count <= 0) {
    list_add_tail(&(current()->list),&sem[n_sem].blocked);
    sched_next_rr();
  }
  if (sem[n_sem].destroyed == 1) return -1;
  else  return 0;
}

int sem_signal (int n_sem) {
  if (n_sem < 0 || n_sem > NR_SEM) return -1;
  sem[n_sem].count ++;
  if (sem[n_sem].count <= 0) {
    struct list_head *l = list_first(&sem[n_sem].blocked);
    list_del(l);
    list_add_tail(l,&readyqueue);
  }
  return 0;
}

int sem_destroy (int n_sem) {
  if (n_sem < 0 || n_sem >= NR_SEM) return -1;
  if (sem[n_sem].count == NULL) return -1;
  while(!list_empty(&sem[n_sem].blocked)) {
    struct list_head *l = list_first(&sem[n_sem].blocked);
    list_del(l);
    sem[n_sem].destroyed = 1;
    list_add_tail(l,&readyqueue);
  }
  list_del(&sem[n_sem].list);
  list_add_tail(&sem[n_sem].list,&freesem_queue);
  return 0;
}

int free_sem(int n_sem) {
  list_add_tail(&sem[n_sem].list,&freesem_queue);
  return 0;
}

int unblock_waiters(int n_sem) {
  if (n_sem < 0 || n_sem >= NR_SEM) return -1;
  if (sem[n_sem].count == NULL) return -1;
  while (!list_empty(&sem[n_sem].blocked)) {
      sem_signal(n_sem);
  }
  return 0;
}


int is_sem_empty()
{
  return list_empty(&freesem_queue);
}

int are_2_free_sem()
{
  return list_at_least_2(&freesem_queue);
}
