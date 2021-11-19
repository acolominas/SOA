#ifndef UTILS_H
#define UTILS_H

#include <list.h>
#include <sched.h>

#define NR_SEM 10

void copy_data(void *start, void *dest, int size);
int copy_from_user(void *start, void *dest, int size);
int copy_to_user(void *start, void *dest, int size);

#define VERIFY_READ	0
#define VERIFY_WRITE	1
int access_ok(int type, const void *addr, unsigned long size);

#define min(a,b)	(a<b?a:b)

unsigned long get_ticks(void);

void memset(void *s, unsigned char c, int size);

struct sem_t {
	//lo ponemos al principio para poder hacer cast directo. La @direccion ya esta la propia struct.
	struct list_head list;
	int id;
	int count;
	//lista de PCB bloqueados
	struct list_head blocked;
};

int sem_init (int n_sem, unsigned int value);  

int sem_wait (int n_sem);

int sem_signal (int n_sem);

int sem_destroy (int n_sem);

int get_free_sem();

int get_sem_id(struct list_head *sem);

#endif
