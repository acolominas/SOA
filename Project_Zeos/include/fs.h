#include <list.h>

#include <sched.h>

#include <types.h>


int get_id(struct list_head *t);

//Tabla Ficheros Abiertos

void init_tfa();

int get_free_tfae();

int free_tfae(int tfae);

//Tabla Canales

void init_tc(struct task_struct *t);

int get_free_tce(struct task_struct * current);

int free_tce(int tce);
