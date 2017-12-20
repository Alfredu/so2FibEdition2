#include <io.h>
#include <utils.h>
#include <list.h>
#include <circularBuffer.h>
#include <sched.h>
// Queue for blocked processes in I/O 
struct list_head blocked;

int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}

int sys_read_keyboard(char *buffer, int size)
{
  struct task_struct *current_tu = current();

  if(list_empty(&keyboardqueue)){
    //llegim


  }
  else{
    //bloquegem
    current_tu->kb_data.to_read = size;
    current_tu->kb_data.char_buffer = buffer;
    current_tu->kb_data.already_read = 0;
    update_process_state_rr(current_tu, &blockedqueue);
    sched_next_rr();

  }
  printk("HOLI");
}
