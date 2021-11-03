 /*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
struct list_head free_queue;
struct list_head ready_queue;
struct task_struct *idle_task;

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

  printk("\nSoy IDLE");
	while(1)
	{
  ;
	}
}

void init_idle (void)
{
	struct list_head *l = list_first(&free_queue);
	list_del(l);
	idle_task = list_head_to_task_struct(l);
	idle_task->PID = 0;
  idle_task->quantum = GLOBAL_QUANTUM;

	allocate_DIR(idle_task);

	union task_union *idleu = (union task_union*)idle_task;
	idleu->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;
	idleu->stack[KERNEL_STACK_SIZE-2] = 0;
	idle_task->kernel_esp = (unsigned long)&idleu->stack[KERNEL_STACK_SIZE-2];
}

void init_task1(void)
{
	struct list_head *l = list_first(&free_queue);
	list_del(l);
	struct task_struct *init_task = list_head_to_task_struct(l);
	init_task->PID = 1;
  init_task->quantum = GLOBAL_QUANTUM;
  num_ticks = GLOBAL_QUANTUM;
	allocate_DIR(init_task);
	set_user_pages(init_task);
	union task_union *initu = (union task_union*)init_task;
	tss.esp0 = (unsigned long)&initu->stack[KERNEL_STACK_SIZE];
	writeMsr(0x175,(unsigned long)&initu->stack[KERNEL_STACK_SIZE]);
	set_cr3(init_task->dir_pages_baseAddr);
}


void init_sched()
{
	INIT_LIST_HEAD(&free_queue);
	INIT_LIST_HEAD(&ready_queue);
	int i;
	for (i = 0;  i< NR_TASKS; ++i) {
		list_add(&task[i].task.lista,&free_queue);
	}
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

struct task_struct *list_head_to_task_struct(struct list_head *l) {
  return (struct task_struct*)((unsigned long)l&0xfffff000);
}

void inner_task_switch(union task_union *new){
  //1.Update the pointer to the system stack to point to the stack of new_task.
	tss.esp0 = (unsigned long)&new->stack[KERNEL_STACK_SIZE];
	writeMsr(0x175,(unsigned long)&new->stack[KERNEL_STACK_SIZE]);
  //2.Change the user address space by updating the current page directory:
	set_cr3(new->task.dir_pages_baseAddr);
  //3.Store the current value of the EBP register in the PCB
	current()->kernel_esp = returnEBP();
  //4.Change the current system stack by setting ESP register to point to the stored value in the new PCB.
  //5.Restore the EBP register from the stack.
  writeESP(new->task.kernel_esp);
}

void sched_next_rr() {
  struct task_struct *next;
  if(list_empty(&ready_queue)) {
    next = idle_task;
  }else {
    struct list_head *l = list_first(&ready_queue);
    list_del(l);
    next = list_head_to_task_struct(l);
  }
  union task_union *unext = (union task_union*)next;
  task_switch(unext);
  num_ticks = get_quantum(next);
}

int needs_sched_rr(){
  if ((num_ticks == 0)&&(!list_empty(&ready_queue))) return 1;
  if (num_ticks == 0) num_ticks = get_quantum(current());
  return 0;
}

void update_sched_data_rr() {
  --num_ticks;
}

int get_quantum (struct task_struct *t) {
  return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum) {
  t->quantum = new_quantum;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dest) {
  list_add_tail(&(t->lista),dest);
}

void schedule() {
  update_sched_data_rr();
  if (needs_sched_rr() == 1) {
    update_process_state_rr(current(),&ready_queue);
    printk("\nTask switch!");
    sched_next_rr();
  }
}
