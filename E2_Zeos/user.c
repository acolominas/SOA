#include <libc.h>



int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  //perror();
  //write(1,"hola",-1);
  //perror();
  //int time = 0;
  char buff[256];

	//time = gettime();
	//itoa(time,budff);
	//write(1,buff,strlen(buff));
  //pid = getpid();
  //itoa(pid,buff);
  //write(1,buff,strlen(buff));
  pid = fork();
  itoa(pid,buff);
  write(1,buff,strlen(buff));
  while(1) { }
}
