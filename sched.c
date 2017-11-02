/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

struct list_head freequeue;
struct list_head readyqueue;
union task_union * task1;
struct task_struct *task1_task;
unsigned int nextPid;
union task_union *provaFork;
extern struct list_head blocked;

struct task_struct * idle_task;

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
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
	;
	}
}

void init_idle (void)
{
	struct list_head * process_idle_list_head = list_first(&freequeue);
	list_del(process_idle_list_head);
	union task_union * process_idle_task_union = (union task_union *)list_head_to_task_struct(process_idle_list_head);
	process_idle_task_union->task.PID = 0;
	allocate_DIR(&process_idle_task_union->task);
	process_idle_task_union->stack[KERNEL_STACK_SIZE-1] = &cpu_idle;
	process_idle_task_union->stack[KERNEL_STACK_SIZE-2] = 0;
	process_idle_task_union->task.kernel_esp = &process_idle_task_union->stack[KERNEL_STACK_SIZE-2];
	idle_task = &process_idle_task_union->task;

}

void init_task1(void)
{
	struct list_head * process_task1_list_head = list_first(&freequeue);
	list_del(process_task1_list_head);	
	union task_union * process_task1_task_union = (union task_union *)list_head_to_task_struct(process_task1_list_head);
	process_task1_task_union->task.PID = 1;
	nextPid = 13;
	allocate_DIR(&process_task1_task_union->task);
	set_user_pages(&process_task1_task_union->task);	
	tss.esp0 = KERNEL_ESP(process_task1_task_union);
	set_cr3(get_DIR(&process_task1_task_union->task));
	task1_task = &process_task1_task_union->task;
}


void init_sched(){
	INIT_LIST_HEAD(&freequeue);
	int i;
	for(i=0; i<NR_TASKS;i++){
		list_add(&(task[i].task.list), &freequeue);
	}
	INIT_LIST_HEAD(&readyqueue);
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

void inner_task_switch(union task_union *new_t){
	tss.esp0 = KERNEL_ESP(new_t);
	set_cr3(new_t->task.dir_pages_baseAddr);
	struct task_struct * current_task = current();	
	unsigned long *ebp;

	__asm__("movl %%ebp, %0;"
		:"=g"(ebp)
	);
	current_task->kernel_esp = ebp;
	asm volatile(
		"movl %0, %%esp;"
		"popl %%ebp;"
		"ret;"
		: 
		: "m" (new_t->task.kernel_esp)
		);

}

void task_switch(union task_union *t) {
	asm("pushl %esi;"
		"pushl %edi;"
		"pushl %ebx;");

	inner_task_switch(t);

	asm("popl %ebx;"
		"popl %edi;"
		"popl %esi;");
}


int getNextPid(){
	nextPid+=1;
	return nextPid;
}


