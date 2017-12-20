/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>
#include <circular_buffer.h>
#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024
#define QUANTUM 300
#define NR_SEMAPHORES 20

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };
struct kbdata{
  int to_read, already_read;
  char *char_buffer;
};
struct task_struct {
  int PID;			/* Process ID. This MUST be the first field of the struct. */
  page_table_entry * dir_pages_baseAddr;
  unsigned long kernel_esp;
  struct list_head list;
  unsigned int quantum;
  enum state_t state;
  struct stats task_stats;
  struct kbdata kb_data;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

/* Semafors */
struct semaphore {
  int counter;
  int owner;
  struct list_head queue;
};
extern struct semaphore semaphores[];
extern int remaining_ticks;
extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task; /* Vector de tasques */
extern union task_union *idle_task;
extern struct task_struct *task1_task;

extern circular_buffer cb;

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct list_head blockedqueue;
extern struct list_head keyboardqueue;

#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

struct task_struct * current();

void task_switch(union task_union*t);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

int get_DIR_pos (page_table_entry *dir);

int getNextPid();
/* Headers for the scheduling policy */
void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();
int get_quantum(struct task_struct *t);
void set_quantum(struct task_struct *t, int new_quantum);

/* Estadística */
void update_stats(unsigned long *v, unsigned long *elapsed);int
get_stats(int pid, struct stats * st);



#endif  /* __SCHED_H__ */


