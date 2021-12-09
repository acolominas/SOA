#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  int pid = fork();
  if (pid == 0) {
    int pd[2];

    int res = pipe(pd);

    char buff[13];
    char buff2[4];

    int res2 = read(pd[0],buff,13);

    itoa(res2,buff2);

    write(1,buff2,strlen(buff2));
    write(1,buff,strlen(buff));
  }

  while(1) { }
}
