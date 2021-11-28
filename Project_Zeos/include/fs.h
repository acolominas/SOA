#include <list.h>

#include <sched.h>

#include <types.h>


int get_id(struct list_head *t);

//Tabla Ficheros Abiertos

void init_tfa();

int get_free_tfae();

int free_tfae(int tfae);

int is_tfae_empty();

//Tabla Canales

void init_tc();

int get_2_free_tce(int *tce_id);

int get_free_tce();

int free_tce(int tce);

int are_2_free_tce();
