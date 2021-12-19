#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  /*
  int pd[2];
  int res = pipe(pd);

  int a = fork();
  if (a != 0) {
    char buff[4];
    itoa(pd[0],buff);
    write(1,buff,strlen(buff));

    itoa(pd[1],buff);
    write(1,buff,strlen(buff));
    write(1,"Padre",5);
    close(pd[0]);
    char buff_write[2] = "AB";
    write(pd[1],buff_write,2);
    close(pd[1]);
  }else {
    char buff[4];
    itoa(pd[0],buff);
    write(1,buff,strlen(buff));

    itoa(pd[1],buff);
    write(1,buff,strlen(buff));

    write(1,"Hijo",4);
    close(pd[1]);
    char buff_read[2];
    read(pd[0],buff_read,2);
    write(1,buff_read,2);
    close(pd[0]);
  }*/
  int a = fork();
  int pd[2];
  int res = pipe(pd);

  if (a > 0) {
    char buff[4] = "HOLA";
    write(pd[1],buff,4);
  }
  else {
    char buff_read[4];
    read(pd[0],buff_read,4);
    write(1,buff_read,4);
 }
  while(1) { }
}
