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

//Nos devuelve el identificador de la primera entrada libre de la TFA
int get_free_tfae() {
   int a = -1;
   if (!list_empty(&tfafreequeue)) {
    struct list_head *tfae = list_first(&tfafreequeue);
    a = get_id(tfae);
    list_del(tfae);
   }
  return a;
}

//Nos indica si existen entradas libres en la TFA
int is_tfae_empty()
{
  return list_empty(&tfafreequeue);
}

//Inicializa la tabla de ficheros abierto
int free_tfae(int tfae)
{
  if (tfae < 0 || tfae > NUM_FICHEROS_ABIERTOS) return -1;
  tfa_array[tfae].nrefs_read = 0;
  tfa_array[tfae].nrefs_write = 0;
  list_add_tail(&tfa_array[tfae].list,&tfafreequeue);
  return 0;
}

//Inicializa la tabla de canales para un proceso dado su task_struct
void init_tc(struct task_struct *t)
{
  int i;
  INIT_LIST_HEAD(&(t->tcfreequeue));

  //CANALES DEFAULT
  tfa_array[0].nrefs_write = 1;
  tfa_array[0].nrefs_read = 1;
  t->tc_array[0].pos = 0;
  t->tc_array[1].pos = 1;
  t->tc_array[0].le = LECTURA;
  t->tc_array[1].le = ESCRIPTURA;
  t->tc_array[0].tfa_entry = (tabla_ficheros_abiertos_entry*) &tfa_array[0];
  t->tc_array[1].tfa_entry = (tabla_ficheros_abiertos_entry*) &tfa_array[0];

  for (i=2; i<NUM_CANALES; i++)
  {
    t->tc_array[i].pos = i;
    t->tc_array[i].le = -1; // No utilizado.
    list_add_tail(&t->tc_array[i].list, &(t->tcfreequeue));
  }
}

//Nos retorna el identificador de un canal libre.
int get_free_tce() {
   int a = -1;
   struct task_struct * t = current();
   if (!list_empty(&t->tcfreequeue)) {
    struct list_head *tce = list_first(&t->tcfreequeue);
    a = get_id(tce);
    list_del(tce);
   }
  return a;
}

//Nos retorna en tce_id los identificadores de dos canales libres.
int get_2_free_tce(int *tce_id) {
  tce_id[0] = -1;
  tce_id[1] = -1;
  struct task_struct *t = current();
  struct list_head *tce;
  if (!list_empty(&(t->tcfreequeue))) {
    tce = list_first(&(t->tcfreequeue));
    tce_id[0] = get_id(tce);
    list_del(tce);
    if (!list_empty(&t->tcfreequeue)) {
      tce = list_first(&t->tcfreequeue);
      tce_id[1] = get_id(tce);
      list_del(tce);
    }
    else {
      list_add_tail(tce,&(t->tcfreequeue));
      return -1;
    }
  }
  else {
    return -1;
  }
 return 0;
}

//Nos devuelve si existen 2 canales libres en la Tabla de Canales del proceso actual.
int are_2_free_tce()
{
  struct task_struct * t = current();
  return list_at_least_2(&t->tcfreequeue);
}

//Libera un canal de la TC_ARRAY del proceso actual dado el identificador de canal tce
//Si el canal era el ultimo apuntador a la entrada de la TFA tambien liberamos la entrada en la TFA.
int free_tce(int tce)
{
  if (tce < 0 || tce > NUM_CANALES) return -1;
  if (current()->tc_array[tce].le == 0) current()->tc_array[tce].tfa_entry->nrefs_read--;
  if (current()->tc_array[tce].le == 1) current()->tc_array[tce].tfa_entry->nrefs_write--;

  current()->tc_array[tce].le = -1; // marcamos el canal como no usado
  page_table_entry *process_PT = get_PT(current());
  int frame = get_frame(process_PT,tfa_array[current()->tc_array[tce].tfa_entry->id_ph_page].id_ph_page);
  del_ss_pag(process_PT, frame);
  //si no hay canales apuntando a la TFAE, la liberamos tambien.
  if (current()->tc_array[tce].tfa_entry->nrefs_read == 0 && current()->tc_array[tce].tfa_entry->nrefs_write == 0) {
    current()->tc_array[tce].tfa_entry = NULL;
    free_tfae(current()->tc_array[tce].tfa_entry->pos);
    get_frame(frame);
  }
  list_add_tail(&current()->tc_array[tce].list,&(current()->tcfreequeue));
  return 0;
}

int check_fd(int fd, int permissions)
{
  if (fd < 0 || fd > NUM_CANALES) return -EBADF;
  if (current()->tc_array[fd].le != permissions) return -EACCES;
  if (permissions == LECTURA && current()->tc_array[fd].tfa_entry->nrefs_write == 0) return 0;
  if (permissions == ESCRIPTURA && current()->tc_array[fd].tfa_entry->nrefs_read == 0) return -EPIPE;
  return 1;
}
