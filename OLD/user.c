#include <libc.h>
#include <p_stats.h>

#define RR 0
#define FCFS 1

char buff[24];

int pid;


int fibonacci(int n)
{
  if (n == 0) return 0;
  else if (n == 1) return 1;
  else return (fibonacci(n-1) + fibonacci(n-2));
}

void write_stats(int pid,struct stats proc)
{  
  int tam = 5;
  char buff_pid[5];
  char buff[tam];
  write(1,"\n",1);
  ;
  if (pid == 0) write(1,"idle ",5);
  else { 
  	itoa(pid,buff_pid);
  	write(1,buff_pid,5);
  }
  itoa(proc.user_ticks,buff);
  write(1,buff,tam);
  itoa(proc.system_ticks,buff);
  write(1,buff,tam);
  itoa(proc.blocked_ticks,buff);
  write(1,buff,tam);
  itoa(proc.ready_ticks,buff);
  write(1,buff,tam);
}

int bucle(int tam)
{
  int total = 0;
  for(int k = 0; k < tam; ++k)
    for(int l = 0; l < tam; ++l)
      ++total;
  return total;
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  int res = 0;
  int total = 0;
  int tam = 10000;
  int ticks_blocked = 1;
  struct stats p_stats;
  res = set_sched_policy(FCFS);
  write(1,"\n",1);
  write(1,"PROC ",5);
  write(1,"USER ",5);
  write(1,"SYS ",4);
  write(1,"BLOCK ",6);
  write(1,"READY ",6);
  if (res < 0)   write(1,"error",5);
  else {
    char buffer[8];    
    int i;
    for(i = 0; i < 3; ++i) {
      int id = fork();
      //itoa(id,buffer);
      //write(1,buffer,5);
      if (id == 0){
        //total = bucle(tam);
        read(0," ",ticks_blocked);
        int f = fibonacci(25);
        int pid = getpid();
        get_stats(pid,&p_stats);
        write_stats(pid,p_stats);
        exit();
      }
    }
  }
  read(0,"",ticks_blocked);
  get_stats(0,&p_stats);
  write_stats(0,p_stats);
  while(1);
