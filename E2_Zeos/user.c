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
  //char buff[4];

	//time = gettime();
	//itoa(time,budff);
	//write(1,buff,strlen(buff));
  //
  //itoa(pid,buff);
  //write(1,buff,strlen(buff));

  int pid = fork();
  if(pid == 0) {
    int pid_2;
    pid_2 = fork();
    int i = 0;
    if(pid_2 != 0) {
      char buff[] = "\nSoy el hijo de init";
      while(1) {
        write(1,buff,strlen(buff));
      }
    }
    else {
      char buff[] = "\nSoy el nieto de init";
      while(1){
        write(1,buff,strlen(buff));
        if (i == 50000) exit();
        i++;
      }
    }
  }
  while(1) { }
}
