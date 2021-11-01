# 1 "entry.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "entry.S"




# 1 "include/asm.h" 1
# 6 "entry.S" 2
# 1 "include/segment.h" 1
# 7 "entry.S" 2
# 72 "entry.S"
.globl keyboard_handler; .type keyboard_handler, @function; .align 0; keyboard_handler:
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %edx; pushl %ecx; pushl %ebx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
 call keyboard_routine
 movb $0x20, %al ; outb %al, $0x20 ;
 popl %ebx; popl %ecx; popl %edx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
 iret

.globl clock_handler; .type clock_handler, @function; .align 0; clock_handler:
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %edx; pushl %ecx; pushl %ebx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
  movb $0x20, %al ; outb %al, $0x20 ;
 call clock_routine
 popl %ebx; popl %ecx; popl %edx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
 iret

.globl syscall_handler_sysenter; .type syscall_handler_sysenter, @function; .align 0; syscall_handler_sysenter:
 push $0x2B
 push %EBP
 pushfl
 push $0x23
 push 4(%EBP)
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %edx; pushl %ecx; pushl %ebx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
 cmpl $0, %EAX
 jl sysenter_err
 cmpl $MAX_SYSCALL, %EAX
 jg sysenter_err
 call *sys_call_table(, %EAX, 0x04)
 jmp sysenter_fin
sysenter_err:
 movl $-38, %EAX
sysenter_fin:
 movl %EAX, 0x18(%ESP)
 popl %ebx; popl %ecx; popl %edx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
 movl (%ESP), %EDX
 movl 12(%ESP), %ECX
 sti
 sysexit

.globl writeMsr; .type writeMsr, @function; .align 0; writeMsr:
 push %ebp
 movl %esp, %ebp
 movl 8(%ebp), %ecx
 movl 12(%ebp), %eax
 movl $0, %edx
 wrmsr
  movl %ebp, %esp
 pop %ebp
 ret

.globl task_switch; .type task_switch, @function; .align 0; task_switch:
 push %ebp
 movl %esp, %ebp;
 pushl %esi
 pushl %edi
 pushl %ebx
 pushl 8(%ebp)
 call inner_task_switch
  addl $4,%esp
 popl %ebx
 popl %edi
 popl %esi
 movl %ebp, %esp
 popl %ebp
 ret

.globl returnEBP; .type returnEBP, @function; .align 0; returnEBP:
 movl %ebp, %eax
 ret

.globl writeESP; .type writeESP, @function; .align 0; writeESP:
 movl 4(%esp),%esp
 popl %ebp
  ret

.globl switch_stack; .type switch_stack, @function; .align 0; switch_stack:
 mov 4(%esp), %eax
 mov %ebp, (%eax)
 mov 8(%esp), %esp
 popl %ebp
 ret
