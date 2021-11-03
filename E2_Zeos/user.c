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
  char buff[4];

	//time = gettime();
	//itoa(time,budff);
	//write(1,buff,strlen(buff));
  //
  //itoa(pid,buff);
  //write(1,buff,strlen(buff));

  pid = fork();
  if(pid == 0) {
    int pid_2 = fork();
    if (pid_2 == 0) {
        char buff1[] = "\nSoy el nieto de init";
        int i;
        for (i = 0; i < 5000; i++ ) {
          write(1,buff1,strlen(buff1));
        }
        //exit();
    }
    char buff[] = "\nSoy el hijo de init";
    int i;
    for (i = 0; i < 5000; i++ ) {
      write(1,buff,strlen(buff));
    }
    exit();
  }
  else {
    char buff[] = "\nSoy init";
    while (1) {
      write(1,buff,strlen(buff));
    }
  }
  while(1) { }
}
