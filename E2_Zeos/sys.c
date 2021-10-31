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
struct list_head free_queue;

int check_fd(int fd, int permissions)
{
  //if (fd!=1) return -9; /*EBADF*/
  //if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  if (fd!=1) return -EBADF;
  if (permissions!=ESCRIPTURA) return -EACCES;
  return 0;
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
  int PID=-1;
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





  // creates the child process

  return PID;
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
