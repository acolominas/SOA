# 1 "wrapper.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "wrapper.S"

# 1 "include/asm.h" 1
# 3 "wrapper.S" 2


.globl write; .type write, @function; .align 0; write:

 pushl %ebp;
 movl %esp, %ebp;
  pushl %ebx


 movl 8(%ebp), %ebx
 movl 12(%ebp), %ecx
 movl 16(%ebp), %edx

 movl $0x04, %eax;

 pushl %ecx;
 pushl %edx;


 pushl $SYSENTER_WRITE

 pushl %ebp
 movl %esp, %ebp

 sysenter;

SYSENTER_WRITE:
                    popl %ebp;
                    popl %edx;
                    popl %edx;
                    popl %ecx;
                    popl %ebx;

if: cmpl $0, %eax;
     jge else;
     neg %eax;
     movl %eax, errno;
     movl $-1, %eax;
else:
     popl %ebp
    ret;




.globl gettime; .type gettime, @function; .align 0; gettime:

pushl %ebp;
movl %esp, %ebp;

movl $10, %eax;

pushl %ecx;
pushl %edx;

pushl $SYSENTER_GETTIME

push %ebp
movl %esp, %ebp

sysenter;

SYSENTER_GETTIME:
                  popl %ebp;
                  popl %edx;
                  popl %edx;
                  popl %ecx;
                  popl %ebp;
                  ret;


.globl getpid; .type getpid, @function; .align 0; getpid:

pushl %ebp;
movl %esp, %ebp;

movl $20, %eax;

pushl %ecx;
pushl %edx;

pushl $SYSENTER_GETPID

push %ebp
movl %esp, %ebp

sysenter;

SYSENTER_GETPID:
      popl %ebp;
      popl %edx;
      popl %edx;
      popl %ecx;
      popl %ebp;
   ret;


.globl fork; .type fork, @function; .align 0; fork:

push %ebp
movl %esp, %ebp

movl $0x02, %eax

pushl %ecx;
pushl %edx;

pushl $SYSENTER_FORK

pushl %ebp
movl %esp, %ebp

sysenter;

SYSENTER_FORK:
 popl %ebp;
 popl %edx;
 popl %edx;
 popl %ecx;
 popl %ebp;
 ret;

iffork: cmpl $0, %eax;
 jge elsefork;
 neg %eax;
 movl %eax, errno;
 movl $-1, %eax;
elsefork:
 popl %ebp
 ret;
