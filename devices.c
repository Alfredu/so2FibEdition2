#include <io.h>
#include <utils.h>
#include <list.h>
#include <circular_buffer.h>
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
  current_tu->kb_data.to_read = size;
  current_tu->kb_data.already_read = 0;
  while(current_tu->kb_data.to_read > 0){
    char key;
    int i=0;
    if(list_empty(&keyboardqueue)){
        //llegim
      if(size <= circular_buf_num_elems(&cb)){
        //legim i tal dia farà un any
        current_tu->kb_data.already_read += size;
        current_tu->kb_data.to_read -= size;
        while(i<size){
          circular_buf_read(&cb, &key);
          copy_to_user(&key, &buffer[i], 1);
          i++;
        }
      }
      else{
        //Si el buffer esta ple, el buidem i bloquegem
        if(circular_buf_full(&cb)){
          current_tu->kb_data.already_read += circular_buf_num_elems(&cb);
          current_tu->kb_data.to_read -= circular_buf_num_elems(&cb);
          while(!circular_buf_empty(&cb)){
            circular_buf_read(&cb, &key);
            copy_to_user(&key, &buffer[i], 1);
            i++;
          }
        }
        update_process_state_rr(current_tu, &keyboardqueue);
        sched_next_rr();
      }
    }
    else{
      //bloquegem
      update_process_state_rr(current_tu, &keyboardqueue);
      sched_next_rr();
    }
  }
  
  return current_tu->kb_data.already_read;
}
