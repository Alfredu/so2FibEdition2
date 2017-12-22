/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
); * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

circular_buffer cb;

struct list_head freequeue;
struct list_head readyqueue;
struct list_head blockedqueue;
struct list_head keyboardqueue;
union task_union * task1;
struct task_struct *task1_task;
unsigned int nextPid;
union task_union *provaFork;

union task_union *idle_task;
int remaining_ticks;

/* Semafors */
struct semaphore semaphores[20];
//int used_semaphores[20] = {0};



int get_quantum(struct task_struct *t){
	return t->quantum; 
}

void set_quantum(struct task_struct *t, int new_quantum){
	t->quantum = new_quantum;	
}

void sched_next_rr(void){
	struct task_struct *t;
	if (list_empty(&readyqueue)) {
		t = &idle_task->task;
	} else {
		struct list_head *next_process = list_first(&readyqueue);
		list_del(next_process);
		t = list_head_to_task_struct(next_process);
	}
	t->state = ST_RUN;
	remaining_ticks = get_quantum(t);
	update_stats(&(current()->task_stats.system_ticks), &(current()->task_stats.elapsed_total_ticks));
	update_stats(&(current()->task_stats.ready_ticks), &(current()->task_stats.elapsed_total_ticks));
	//actualitzar estats
	task_switch((union task_union*)t);
}
void update_sched_data_rr (void){
	remaining_ticks--;
	if(needs_sched_rr()){
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}

int needs_sched_rr(void){
	if(remaining_ticks<=0 && !list_empty(&readyqueue)){
		return 1;
	}
	if (remaining_ticks==0) remaining_ticks=get_quantum(current());
	return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dest){
	if(t->state!=ST_RUN){
		//treiem el prusés de la cua on es
		list_del(&t->list);
	}
	if (dest!=NULL) {
		list_add_tail(&t->list, dest);
		if(dest!=&readyqueue){
			t->state = ST_BLOCKED;
		}
		else{
			update_stats(&(t->task_stats), &(t->task_stats.elapsed_total_ticks));
			t->state = ST_READY;
		}
	}
	else t->state=ST_RUN;
}

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

int get_DIR_pos (page_table_entry *dir) 
{
	int dirPos = ((unsigned int)dir - (unsigned int)&dir_pages) / (TOTAL_PAGES * sizeof(page_table_entry));
	return dirPos;
}


int allocate_DIR(struct task_struct *t) 
{
	int pos = 0;

	// pos = ((int)t-(int)task)/sizeof(union task_union);

	while(pos<NR_TASKS && dir_references[pos] != 0){
		pos++;
	}
	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];
	dir_references[pos]++;
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
void init_task_stats(struct stats *task_stats)
{
	task_stats->blocked_ticks=0;
	task_stats->elapsed_total_ticks=get_ticks();
	task_stats->ready_ticks=0;
	task_stats->remaining_ticks = get_ticks(); //DE CADA PRUSÉS?
	task_stats->system_ticks=0;
	task_stats->total_trans=0;
	task_stats->user_ticks=0;
	
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
	idle_task = process_idle_task_union;

	//STATS
	init_task_stats(&process_idle_task_union->task.task_stats);
	
}

void init_task1(void)
{
	struct list_head * process_task1_list_head = list_first(&freequeue);
	list_del(process_task1_list_head);	
	union task_union * process_task1_task_union = (union task_union *)list_head_to_task_struct(process_task1_list_head);
	process_task1_task_union->task.PID = 1;
	nextPid = 13;
	process_task1_task_union->task.quantum = QUANTUM;
	process_task1_task_union->task.state = ST_RUN;
	allocate_DIR(&process_task1_task_union->task);
	set_user_pages(&process_task1_task_union->task);	
	tss.esp0 = KERNEL_ESP(process_task1_task_union);
	set_cr3(get_DIR(&process_task1_task_union->task));
	task1_task = &process_task1_task_union->task;
	remaining_ticks = process_task1_task_union->task.quantum;

	//STATS
	init_task_stats(&process_task1_task_union->task.task_stats);
	process_task1_task_union->task.program_break = (void *)((PAG_LOG_INIT_DATA+NUM_PAG_DATA)*PAGE_SIZE);
}


void init_sched(){
	INIT_LIST_HEAD(&freequeue);
	int i;
	for(i=0; i<NR_TASKS;i++){
		list_add(&(task[i].task.list), &freequeue);
	}
	INIT_LIST_HEAD(&readyqueue);
	INIT_LIST_HEAD(&blockedqueue);
	INIT_LIST_HEAD(&keyboardqueue);
	for(i=0; i<NR_SEMAPHORES; i++) {
		semaphores[i].owner = -1;
	}
	circular_buffer_init(&cb, 4096);
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
	
	struct task_struct * current_task = current();
	tss.esp0 = KERNEL_ESP(new_t);
	if(get_DIR_pos(get_DIR(&new_t->task)) != get_DIR_pos(get_DIR(current_task))){
		set_cr3(new_t->task.dir_pages_baseAddr);
	}	
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
		"popl %ebx;"
		"popl %edi;"
		"popl %esi;");	
}


int getNextPid(){
	nextPid+=1;
	return nextPid;
}


void update_stats(unsigned long *v, unsigned long *elapsed)
{
	unsigned long current_ticks;
	
	current_ticks=get_ticks();
	
	*v += current_ticks - *elapsed;
	
	*elapsed=current_ticks;
	
}


