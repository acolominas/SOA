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
  allocate_DIR(&hijo);
  //Obtenemos tabla de paginas para el hijo
  page_table_entry *TP_hijo = get_PT(&uhijo->task);
  int i,page_log;

  //Paginas para data+stack

  for (page_log=0; page_log<NUM_PAG_DATA; page_log++) {
    int page_id_fis = alloc_frame();
    if (page_id_fis == -1) {
      //LIBERAR RECURSOS
      for (i=0; i<page_log; i++)
      {
        free_frame(get_frame(TP_hijo, PAG_LOG_INIT_DATA+i));
        del_ss_pag(TP_hijo, PAG_LOG_INIT_DATA+i);
      }
      list_add(hijo, &free_queue);
      return -ENOMEM;
    }
    else {
      set_ss_pag(TP_hijo, PAG_LOG_INIT_DATA+page_log,page_id_fis);
    }
  }
  //Obtenemos tabla de paginas del padre
  page_table_entry *TP_padre = get_PT(current());
  //Las paginas del Kernel del hijo apuntan a las del padre
  for (page_log=0; page_log<NUM_PAG_KERNEL; page_log++){
    set_ss_pag(TP_hijo,page_log, get_frame(TP_padre, page_log));
  }
  //Las paginas del codigo del hijo apuntan a las del padre
  for (page_log=0; page_log<NUM_PAG_CODE; page_log++){
    set_ss_pag(TP_padre, PAG_LOG_INIT_CODE+page_log, get_frame(TP_hijo, PAG_LOG_INIT_CODE+page_log));
  }

  // Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to

  for (page_log=NUM_PAG_KERNEL+NUM_PAG_CODE; page_log<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; page_log++)
  {
    //Map one child page to parent's address space.
    int page_log_hijo = get_frame(TP_hijo,page_log);
    set_ss_pag(TP_padre, page_log+NUM_PAG_DATA,page_log_hijo);
    copy_data((void*)(page_log<<12), (void*)((page_log+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(TP_padre, page_log+NUM_PAG_DATA);
  }

  //flush TLB
  set_cr3(get_DIR(current()));
  uhijo->task.PID = newPID();

  //16 pos to contx hw & sw, 17 -> @handler,18->@ret_from_fork,19->0 to ebp task_switch
  uhijo->stack[KERNEL_STACK_SIZE-19] = 0;
  uhijo->stack[KERNEL_STACK_SIZE-18] = (unsigned long)&ret_from_fork;
  hijo->kernel_esp = (unsigned long *)&uhijo->stack[KERNEL_STACK_SIZE-19];

  list_add_tail(&(uhijo->task.lista), &ready_queue);

  return uhijo->task.PID;
}

void sys_exit()
{
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
