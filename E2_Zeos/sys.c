/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1



extern int zeos_ticks;
extern struct list_head free_queue;
extern struct list_head ready_queue;

int PID = 500;

int check_fd(int fd, int permissions)
{
  //if (fd!=1) return -9; /*EBADF*/
  //if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  if (fd!=1) return -EBADF;
  if (permissions!=ESCRIPTURA) return -EACCES;
  return 0;
}

int ret_from_fork() {
	return 0;
}

int newPID() {
  PID++;
  return PID;
}

int sys_ni_syscall()
{
	//return -38; /*ENOSYS*/
  return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  //1.Get a free task_struct for the process. If there is no space for a new process, an error will be returned.

  if(list_empty(&free_queue)) return -ENOMEM;
  struct list_head *l = list_first(&free_queue);
  struct task_struct *hijo = list_head_to_task_struct(l);
  union task_union *uhijo = (union task_union*)hijo;
  list_del(l);

  //2.copy the parent’s task_union to the child.
  copy_data(current(),hijo,sizeof(union task_union));
  //3.Initialize field dir_pages_baseAddr with a new directory to store the process address space using the allocate_DIR routine.
  allocate_DIR(hijo);

  //Obtenemos paginas fisicas para data+stack, sino hay mas, error.
  int pages_data[NUM_PAG_DATA];
  int page_log,page_ph;
  for (page_log=0; page_log<NUM_PAG_DATA; page_log++) {
    page_ph = alloc_frame();
    if (page_ph == -1) {
      return -ENOMEM;
    }
    else {
      pages_data[page_log] = page_ph;
    }
  }

  //Obtenemos tabla de paginas para el hijo y padre
  page_table_entry *TP_hijo = get_PT(hijo);
  page_table_entry *TP_padre = get_PT(current());

  //Las paginas del Kernel del hijo apuntan a las del padre (Es compartido)
  //Las paginas logicas del kernel son las primeras (empiezan en 0)
  for (page_log=0; page_log<NUM_PAG_KERNEL; page_log++){
    page_ph = get_frame(TP_padre, page_log);
    set_ss_pag(TP_hijo,page_log,page_ph);
  }
  //Las paginas del codigo del hijo apuntan a las del padre (Es compartido)
  for (page_log=0; page_log<NUM_PAG_CODE; page_log++){
    page_ph = get_frame(TP_padre, PAG_LOG_INIT_CODE+page_log);
    set_ss_pag(TP_hijo,PAG_LOG_INIT_CODE+page_log,page_ph);
  }

  //Añadimos a la TP del hijo las nuevas paginas para DATA
  for (page_log = 0; page_log < NUM_PAG_DATA; page_log++) {
    //set_ss_pag(Tabla_pagina,id_pagina_logica,id_pagina_fisica)
		set_ss_pag(TP_hijo,PAG_LOG_INIT_DATA+page_log,pages_data[page_log]);
	}

  //Copiamos el contenido de las paginas de DATA+STACK del padre a las paginas del hijo.
  int dir_src,dir_dest;
  int page_ph_hijo;
  int i = 0;
  //LAS PAGINAS FISICAS DEL HIJO LAS PONEMOS AL FINAL DE LA TP DEL PADRE
  int pos = PAG_LOG_INIT_DATA+NUM_PAG_DATA;
  for (page_log=0; page_log<NUM_PAG_DATA; page_log++){
    //Para poder acceder, añadimos la pagina a la TP del padre
    //no podemos acceder al espacio de direeciones del hijo desde el padre
    //Solucion: Añadir las paginas del hijo al espacio de direcciones del padre.
    //          Se añaden al final de la TP del Padre
    // TP [KERNEL | CODE | DATA | DATA_HIJO]
    page_ph_hijo = pages_data[i];
    set_ss_pag(TP_padre,pos+page_log,page_ph_hijo);

    dir_src = (PAG_LOG_INIT_DATA+page_log)*PAGE_SIZE;
    dir_dest = (pos+page_log)*PAGE_SIZE;
    copy_data((void*)(dir_src), (void*)(dir_dest), PAGE_SIZE); //direccion logica

    //Eliminamos las pagina de la TP del padre (ahora solo esta en la TP del hijo)
    del_ss_pag(TP_padre,pos+page_log);
    ++i;
  }
  //Every time Page Table is modified, it is necessary to invalidate TLB (TLB Flush)
  //TLB entries can become incorrect
  set_cr3(get_DIR(current()));

  //Asignamos PID al hijo
  hijo->PID = newPID();


  //Preparamos la stack del hijo
  //stack[KERNEL_STACK_SIZE-19] = 0 //pop %ebp task_switch
  //stack[KERNEL_STACK_SIZE-18] = @ret_from_fork
  //stack[KERNEL_STACK_SIZE-17] = @ret_handler
  //stack[KERNEL_STACK_SIZE-16] = CTX SW
  //stack[KERNEL_STACK_SIZE-4] = CTX HW
  uhijo->stack[KERNEL_STACK_SIZE-19] = 0;
  uhijo->stack[KERNEL_STACK_SIZE-18] = (unsigned long)&ret_from_fork;
  hijo->kernel_esp = (unsigned long)&uhijo->stack[KERNEL_STACK_SIZE-19];

  //Encolamos el hijo a la cola de ready
  list_add_tail(&(uhijo->task.lista), &ready_queue);

  return hijo->PID;
}

void sys_exit()
{
  struct task_struct *current_task = current();
  page_table_entry *TP_current = get_PT(current_task);
  //Liberamos las paginas del proceso. Las compartidas no.
  int page_log;
  for (page_log = 0; page_log < NUM_PAG_DATA; page_log++) {
		free_frame(PAG_LOG_INIT_DATA+page_log);
    del_ss_pag(TP_current,PAG_LOG_INIT_DATA+page_log);
	}
  //Liberamos PCB
  list_add_tail(&(current_task->lista), &free_queue);
  //Forzamos un nuevo task_switch
  sched_next_rr();
}

int sys_write(int fd, char * buffer, int size) {
	int error = check_fd(fd,ESCRIPTURA);
	char buff[128];
   if (error < 0) return error;
   if (access_ok(VERIFY_READ,buffer,size) == 0) return -EFAULT;
   if (size <= 0) return -EINVAL;

	int i = size;
	int ret;

	while (i > 128) {
		 copy_from_user(buffer,buff,128);
		 ret = sys_write_console(buff,128);
		 buffer += ret;
		 i -= ret;
	}
	if (i > 0) {
		 copy_from_user(buffer,buff,i);
		 ret = sys_write_console(buff,i);
		 buffer += ret;
		 i -= ret;
	}
  return size-i;
}

int sys_gettime() {
  return zeos_ticks;
}
