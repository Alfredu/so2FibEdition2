/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define BUFFER_SIZE 10
extern int zeos_ticks;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;
  //Gets a free task struct. If none are available, returns ENOMEM
  if(list_empty(&freequeue)) return -ENOMEM;
  struct list_head * child_list_head = list_first(&freequeue);
  list_del(child_list_head);
  union task_union * father_task_union = (union task_union *) current();
  union task_union * child_task_union = (union task_union*)list_head_to_task_struct(child_list_head);
  copy_data(father_task_union, child_task_union, sizeof(union task_union));

  //TODO preguntar al profe que vol dir lo de modify parent table


  allocate_DIR(&child_task_union->task);
  page_table_entry *child_pt = get_PT(&child_task_union->task);

  for(int i=0; i<NUM_PAG_DATA; i++){
	  int pframe = alloc_frame();
	  if(pframe < 0){
		  //ALLIBERAR TOTS ELS FRAMES
		  //ALLIBERAR EL PROCES (AL PRUCÉS)
		  return -ENOMEM;
	  }
	  //The first data logic page begins at PAG_LOG_INIT_DATA
	  set_ss_pag(child_pt, PAG_LOG_INIT_DATA+i, pframe);
  }

  return PID;
}

void sys_exit()
{  
}

int sys_write(int fd, char * buffer, int size) {
	int ret = check_fd(fd, ESCRIPTURA);
	if (ret < 0) return ret;
	if (!buffer) return -EFAULT;
	if (size<0) return -EINVAL;

	char pointer[BUFFER_SIZE];
	while(size > BUFFER_SIZE && ret){
		copy_from_user(buffer, pointer, BUFFER_SIZE);
		ret = sys_write_console(pointer, BUFFER_SIZE);
		size = size - BUFFER_SIZE;
	}
	copy_from_user(buffer, pointer, size);
	ret = sys_write_console(pointer, size);

	return ret;

}

int sys_gettime() 
{
	return zeos_ticks;
}

