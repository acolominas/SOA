/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#include <fs.h>

#include <stddef.h>

#define TAM_BUFFER 512

extern struct list_head tfafreequeue;

extern tabla_ficheros_abiertos_entry tfa_array[NUM_FICHEROS_ABIERTOS];

extern struct sem_t sem[NR_SEM];

void * get_ebp();


int sys_write_standard(char *buffer, int nbytes){
  int bytes_left = nbytes;
  int ret;
  char localbuffer [TAM_BUFFER];
  while (bytes_left > TAM_BUFFER) {
    copy_from_user(buffer, localbuffer, TAM_BUFFER);
    ret = sys_write_console(localbuffer, TAM_BUFFER);
    bytes_left-=ret;
    buffer+=ret;
  }
  if (bytes_left > 0) {
    copy_from_user(buffer, localbuffer,bytes_left);
    ret = sys_write_console(localbuffer, bytes_left);
    bytes_left-=ret;
  }
  return (nbytes-bytes_left);
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;

  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);

  list_del(lhcurrent);

  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);

  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));

  //aumentamos numero refs
  int c;
  for(c = 0; c < NUM_CANALES; ++c) {
    if (current()->tc_array[c].le != -1) {
      current()->tc_array[c].tfa_entry->nrefs_read++;
      current()->tc_array[c].tfa_entry->nrefs_write++;
    }
  }

  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);

  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);

      /* Return error */
      return -EAGAIN;
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }


  int pages = TOTAL_PAGES-1;
  while (parent_PT[pages].bits.present == 1) {
    set_ss_pag(process_PT, pages, get_frame(parent_PT, pages));
    --pages;
  }



  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);

  return uchild->task.PID;
}

int sys_write(int fd, char *buffer, int nbytes) {
    int bytes_left;
    int ret;
  	if ((ret = check_fd(fd, ESCRIPTURA)) <= 0) return ret;
  	if (nbytes < 0)
  		return -EINVAL;
  	if (!access_ok(VERIFY_READ, buffer, nbytes))
  		return -EFAULT;

    if (fd == 1) return sys_write_standard(buffer,nbytes);
    else {
      bytes_left = nbytes;
      //puntero a la posición donde empezar a leer
      //char **pos_write = (char **)&current()->tc_array[fd].tfa_entry->buffer_write;
      char **pos_write = &current()->tc_array[fd].tfa_entry->buffer_write;
      int *bytes = &(current()->tc_array[fd].tfa_entry->bytes);
      int bytes_escritos= 0;
      int sem_id_r = current()->tc_array[fd].tfa_entry->sem_id_r;
      int sem_id_w = current()->tc_array[fd].tfa_entry->sem_id_w;
      //bytes que vamos a escribir

      while (bytes_left > 0){

        unblock_waiters(sem_id_r);

        if(*bytes + bytes_left <= PAGE_SIZE) {
         //copy_from_user(buffer,pos_write,bytes_left);
         copy_data(buffer,*pos_write,bytes_left);
         bytes_escritos += bytes_left;
         buffer += bytes_left;
         *pos_write = *pos_write + (bytes_left % PAGE_SIZE);
         //*pos_write += (bytes_left % PAGE_SIZE);
         *bytes = bytes_escritos;
         bytes_left -= bytes_left;
        }
        else {
         int restant = PAGE_SIZE - *bytes;
         //copy_from_user(buffer,pos_write,restant);
         copy_data(buffer,*pos_write,restant);
         bytes_escritos += restant;
         buffer += restant;
         //*pos_write += (restant % PAGE_SIZE);
         *pos_write = *pos_write + (restant % PAGE_SIZE);
         *bytes = bytes_escritos;
          bytes_left -= restant;
         //aun quedan bytes por escribir, se bloquea hasta que el lector lea y consuma bytes suficiente
         sem_wait(sem_id_w);
       }
     }
    return bytes_escritos;
  }
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }

  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);

  current()->PID=-1;

  //Liberate all used channels
  for(i = 0; i < NUM_CANALES; ++i) {
    if (current()->tc_array[i].le != -1) free_tce(i);
  }

  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;

  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT;

  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}


int sys_pipe(int *pd)
{
  int tfae,new_ph_pag,sem_id_r,sem_id_w;
  if(is_tfae_empty() || !are_2_free_tce() || !are_2_free_sem() ) return -EMFILE;
  else {
    new_ph_pag=alloc_frame();
    if (new_ph_pag != -1) {
      tfae = get_free_tfae();
      get_2_free_tce(pd);

      sem_id_r = get_free_sem();
      sem_id_w = get_free_sem();

      page_table_entry *current_PT = get_PT(current());

      int i = TOTAL_PAGES-1;
      while (current_PT[i].bits.present == 1) --i; // i sera la pagina que estará libre
      current_PT[i].bits.user = 0; // solo accesible en privilegios nivel 0
      set_ss_pag(current_PT,i,new_ph_pag);
      set_cr3(get_DIR(current()));
      tfa_array[tfae].id_ph_page = i;
      tfa_array[tfae].buffer_read = i*PAGE_SIZE;
      tfa_array[tfae].buffer_write = i*PAGE_SIZE;
      tfa_array[tfae].bytes = 0;
      tfa_array[tfae].nrefs_read++;
      tfa_array[tfae].nrefs_write++;
      //tfa_array[tfae].semaforo = sem[sem_id];
      tfa_array[tfae].sem_id_r = sem_id_r;
      tfa_array[tfae].sem_id_w = sem_id_w;

      sem_init(sem_id_r,0);
      sem_init(sem_id_w,0);
      current()->tc_array[pd[0]].le = LECTURA;
      current()->tc_array[pd[1]].le = ESCRIPTURA;
      current()->tc_array[pd[0]].tfa_entry = (tabla_ficheros_abiertos_entry*) &tfa_array[tfae];
      current()->tc_array[pd[1]].tfa_entry = (tabla_ficheros_abiertos_entry*) &tfa_array[tfae];

    }
    else {
      return -ENOMEM;
    }
  }
  return 0;
}

int sys_read(int fd, void *buf, size_t size)
{
  int ret;
  //comprobamos que el canal esté ok:
  //es de LECTURA
  //hay escritore
  /*HAY UN BUFF[64] size 62*/
  //int fd ok

 /*char buffer[20] = "Esto es un testt";
 current()->tc_array[fd].tfa_entry->nrefs_write = 1;
 current()->tc_array[fd].tfa_entry->buffer_read = &buffer;
 current()->tc_array[fd].tfa_entry->bytes = 17;*/

  if ((ret = check_fd(fd, LECTURA)) <= 0) return ret;
  if (!access_ok(VERIFY_WRITE, buf, size))
    return -EFAULT;

  //puntero a la posición donde empezar a leer
  //char **pos_read = (char **)&current()->tc_array[fd].tfa_entry->buffer_read;
  char **pos_read = &current()->tc_array[fd].tfa_entry->buffer_read;
  // #bits que quedan para leer
  int *total_bytes = &(current()->tc_array[fd].tfa_entry->bytes);
  int bytes_left = size;

  int sem_id_w = current()->tc_array[fd].tfa_entry->sem_id_w;
  int sem_id_r = current()->tc_array[fd].tfa_entry->sem_id_r;

  while(bytes_left > 0) {
    if (*total_bytes >= bytes_left ) {
     // copy_from_user(pos_read,buf,bytes_left);
      copy_data(*pos_read,buf,bytes_left);
      *pos_read = *pos_read + (bytes_left % PAGE_SIZE);
      buf = buf + bytes_left;
      *total_bytes -= bytes_left;
      bytes_left = 0;

      unblock_waiters(sem_id_w);
    }
    else if (*total_bytes < bytes_left && *total_bytes != 0) {
      //copy_from_user(pos_read,buf,total_bytes);
      copy_data(*pos_read,buf,*total_bytes);
      bytes_left -= *total_bytes;
      *pos_read = *pos_read + (*total_bytes % PAGE_SIZE);
      buf += *total_bytes;
      *total_bytes = 0;
      unblock_waiters(sem_id_w);
    }
    else if (*total_bytes == 0 && bytes_left > 0) {
      //si el numero de bytes de la entrada de la tfa apuntada por el canal del proceso es 0
      sem_wait(sem_id_r);

    }
  }

  return (size - bytes_left);

}

int sys_close(int fd)
{
  int res = free_tce(fd);
  return res;
}
