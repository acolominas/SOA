/*
 * libc.c
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;

  if (a==0) { b[0]='0'; b[1]=0; return ;}

  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }

  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;

  i=0;

  while (a[i]!=0) i++;

  return i;
}

void perror() {
  switch(errno) {
    case EBADF:
      write(1, "Bad file number", strlen("Bad file number"));
      break;
    case EACCES:
      write(1, "Permission denied", strlen("Permission denied"));
      break;
    case EFAULT:
      write(1, "Bad address", strlen("Bad address"));
      break;
    case ENOMEM:
      write(1, "Not enough core", strlen("Not enough core"));
      break;
  }
}

