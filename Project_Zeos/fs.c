#include <fs.h>

struct list_head tfafreequeue;

tabla_ficheros_abiertos_entry tfa_array[NUM_FICHEROS_ABIERTOS];


//retorna el ID dado un list_head de tabla_canales_entry o tabla_ficheros_abiertos_entry
int get_id(struct list_head *t)
{
  return *((int*)((int)t+sizeof(struct list_head)));
}

//Tabla Ficheros Abiertos
void init_tfa()
{
  int i;
  INIT_LIST_HEAD(&tfafreequeue);
  for (i=1; i<NUM_FICHEROS_ABIERTOS; i++)
  {
    tfa_array[i].nrefs_read = 0;
    tfa_array[i].nrefs_write = 0;
    tfa_array[i].pos = i;
    list_add_tail(&(tfa_array[i].list), &tfafreequeue);
  }
}

int get_free_tfae() {
   int a = -1;
   if (!list_empty(&tfafreequeue)) {
    struct list_head *tfae = list_first(&tfafreequeue);
    a = get_id(tfae);
    list_del(tfae);
   }
  return a;
}

int free_tfae(int tfae)
{
  if (tfae < 0 || tfae>= NUM_FICHEROS_ABIERTOS) return -1;
  tfa_array[tfae].nrefs_read = 0;
  tfa_array[tfae].nrefs_write = 0;
  list_add_tail(&tfa_array[tfae].list,&tfafreequeue);
  return 0;
}

//Tabla Canales

void init_tc(struct task_struct *t)
{
  int i;
  INIT_LIST_HEAD(&(t->tcfreequeue));

  //CANALES DEFAULT
  tfa_array[0].nrefs_write = 1;
  tfa_array[0].nrefs_read = 1;
  t->tc_array[0].pos = 0;
  t->tc_array[1].pos = 1;
  t->tc_array[0].tfa_entry = (tabla_ficheros_abiertos_entry*) &tfa_array[0];
  t->tc_array[1].tfa_entry = (tabla_ficheros_abiertos_entry*) &tfa_array[0];

  for (i=2; i<NUM_CANALES; i++)
  {
    t->tc_array[i].pos = i;
    list_add_tail(&t->tc_array[i].list, &(t->tcfreequeue));
    //tabla_canales_entry * tce = t->tc_array[i];
    //tce->pos = i;
    //list_add_tail(&tce->list, &(t->tcfreequeue));
  }
}

int get_free_tce(struct task_struct * current) {
   int a = -1;
   if (!list_empty(&current->tcfreequeue)) {
    struct list_head *tce = list_first(&current->tcfreequeue);
    a = get_id(tce);
    list_del(tce);
   }
  return a;
}

int free_tce(int tce)
{
  if (tce < 0 || tce >= NUM_CANALES) return -1;
  list_add_tail(&current()->tc_array[tce].list,&(current()->tcfreequeue));
  return 0;
}
