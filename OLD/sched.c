/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <types.h>
#include <hardware.h>
#include <segment.h>
#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>
#include <p_stats.h>
#include <schedperf.h>


/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;

// Free task structs
struct list_head freequeue;
// Ready queue
struct list_head readyqueue;

void init_stats(struct stats *s)
{
	s->user_ticks = 0;
	s->system_ticks = 0;
	s->blocked_ticks = 0;
	s->ready_ticks = 0;
	s->elapsed_total_ticks = get_ticks();
	s->total_trans = 0;
	s->remaining_ticks = get_ticks();
}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t)
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	sys_write(1,"idle",4);
	}
}

#define DEFAULT_QUANTUM 10

int remaining_quantum=0;

int get_quantum(struct task_struct *t)
{
  return t->total_quantum;
}

void set_quantum(struct task_struct *t, int new_quantum)
{
  t->total_quantum=new_quantum;
}

struct task_struct *idle_task=NULL;

void update_sched_data_rr(void)
{
  remaining_quantum--;
}

int needs_sched_rr(void)
{
  if ((remaining_quantum==0)&&(!list_empty(&readyqueue))) return 1;
  if (remaining_quantum==0) remaining_quantum=get_quantum(current());
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  if (t->state!=ST_RUN) list_del(&(t->list));
  if (dst_queue!=NULL)
  {
    list_add_tail(&(t->list), dst_queue);
    if (dst_queue!=&readyqueue) t->state=ST_BLOCKED;
    else
    {
      update_stats(&(t->p_stats.system_ticks), &(t->p_stats.elapsed_total_ticks));
      t->state=ST_READY;
    }
  }
  else t->state=ST_RUN;
}

void sched_next_rr(void)
{
  struct list_head *e;
  struct task_struct *t;

  if (!list_empty(&readyqueue)) {
	e = list_first(&readyqueue);
    list_del(e);

    t=list_head_to_task_struct(e);
  }
  else
    t=idle_task;

  t->state=ST_RUN;
  remaining_quantum=get_quantum(t);

  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
  update_stats(&(t->p_stats.ready_ticks), &(t->p_stats.elapsed_total_ticks));
  t->p_stats.total_trans++;

  task_switch((union task_union*)t);
}

void schedule()
{
  update_sched_data();
  if (needs_sched())
  {
    update_process_state(current(), &readyqueue);
    sched_next();
  }
}

void init_idle (void)
{
  struct list_head *l = list_first(&freequeue);
  list_del(l);
  struct task_struct *c = list_head_to_task_struct(l);
  union task_union *uc = (union task_union*)c;

  c->PID=0;

  c->total_quantum=DEFAULT_QUANTUM;

  init_stats(&c->p_stats);

  allocate_DIR(c);

  uc->stack[KERNEL_STACK_SIZE-1]=(unsigned long)&cpu_idle; /* Return address */
  uc->stack[KERNEL_STACK_SIZE-2]=0; /* register ebp */

  c->register_esp=(int)&(uc->stack[KERNEL_STACK_SIZE-2]); /* top of the stack */

  idle_task=c;
}

void init_task1(void)
{
  struct list_head *l = list_first(&freequeue);
  list_del(l);
  struct task_struct *c = list_head_to_task_struct(l);
  union task_union *uc = (union task_union*)c;

  c->PID=1;

  c->total_quantum=DEFAULT_QUANTUM;

  c->state=ST_RUN;

  remaining_quantum=c->total_quantum;

  init_stats(&c->p_stats);

  allocate_DIR(c);

  set_user_pages(c);

  tss.esp0=(DWord)&(uc->stack[KERNEL_STACK_SIZE]);

  set_cr3(c->dir_pages_baseAddr);
}

void init_freequeue()
{
  int i;

  INIT_LIST_HEAD(&freequeue);

  /* Insert all task structs in the freequeue */
  for (i=0; i<NR_TASKS; i++)
  {
    task[i].task.PID=-1;
    list_add_tail(&(task[i].task.list), &freequeue);
  }
}

void init_sched()
{
  init_freequeue();
  INIT_LIST_HEAD(&readyqueue);
  init_sched_policy();
}

struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

struct task_struct* list_head_to_task_struct(struct list_head *l)
{
  return (struct task_struct*)((int)l&0xfffff000);
}

/* Do the magic of a task switch */
void inner_task_switch(union task_union *new)
{
  page_table_entry *new_DIR = get_DIR(&new->task);

  /* Update TSS to make it point to the new stack */
  tss.esp0=(int)&(new->stack[KERNEL_STACK_SIZE]);

  /* TLB flush. New address space */
  set_cr3(new_DIR);

  /* Stores current ebp */
  __asm__ __volatile__ (
  	"movl %%ebp, %0\n\t"
	: "=g" (current()->register_esp)
	: );

  /* Restores new esp. Now, we are in the new process */
  __asm__ __volatile__ (
  	"movl %0, %%esp\n\t"
	:
	: "g" (new->task.register_esp) );

  /* Restores ebp from the stack */
  __asm__ __volatile__ (
  	"popl %%ebp\n\t"
	:
	: );

  /* To prevent returning with compiler's frame recovery, we put our own ret instr */
  __asm__ __volatile__ (
  	"ret\n\t"
	:
	: );
}

/* Given the new process, performs a task switch */
void task_switch(union task_union *new)
{
  __asm__ __volatile__ (
  	"pushl %esi\n\t"
	"pushl %edi\n\t"
	"pushl %ebx\n\t"
	);
  inner_task_switch(new);
  __asm__ __volatile__ (
  	"popl %ebx\n\t"
	"popl %edi\n\t"
	"popl %esi\n\t"
	);
}

/* Force a task switch assuming that the scheduler does not work with priorities */
void force_task_switch()
{
  update_process_state_rr(current(), &readyqueue);

  sched_next_rr();
}

struct stats * get_task_stats(struct task_struct *t)
{ //returns: a pointer to the statistics field in task t
  return &t->p_stats;
}

struct list_head *get_task_list(struct task_struct *t)
{//returns: a pointer to the list_head field in the task t
  return &t->list;
}

void block_process(struct list_head *block_queue) {
  struct task_struct *t = current();
  struct stats *st = get_task_stats(t);
  update_process_state(t, block_queue);
  st->system_ticks = get_ticks()-st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
  sched_next();
}

void unblock_process(struct task_struct *blocked){
  struct stats *st = get_task_stats(blocked);
  struct list_head *l = get_task_list(blocked);
  update_process_state(blocked, &readyqueue);
  st->blocked_ticks += (get_ticks()-st->elapsed_total_ticks);
  st->elapsed_total_ticks = get_ticks();
  if (needs_sched()) {
    update_process_state(current(), &readyqueue);
    sched_next();
  }
}
