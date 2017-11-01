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
  int PID=1;
  //Gets a free task struct. If none are available, returns ENOMEM
  if(list_empty(&freequeue)) return -ENOMEM;

  int child_frames[NUM_PAG_DATA];  
  /*Si assegurem que hi ha prou frames abans de agafar un task_union free
	no cal que despres alliberem el task_union si ens donem compte que no teniem
	prous frames. Gran exito*/
  for(int i=0; i<NUM_PAG_DATA; i++){
	child_frames[i] = alloc_frame();
	if(child_frames[i] < 0){ //ERROR 
		//ALLIBERAR TOTS ELS FRAMES
		for(int j=0; j<i; j++){
			free_frame(child_frames[j]);
		}
		return -ENOMEM;
	}
  }
  struct list_head * child_list_head = list_first(&freequeue);
  list_del(child_list_head);
  union task_union *father_task_union = (union task_union *) current();
  union task_union *child_task_union = (union task_union *)list_head_to_task_struct(child_list_head);
  //Hem reservat el task union del fill i ara copiem el contingut del task union del pare al fill.
  copy_data(father_task_union, child_task_union, sizeof(union task_union));

  //TODO preguntar al profe que vol dir lo de modify parent table.

  allocate_DIR(&child_task_union->task);
  page_table_entry *child_pt = get_PT(&child_task_union->task);
  page_table_entry *father_pt = get_PT(&father_task_union->task);

  /*Ara procedirem a omplir la taula de pagines del fill. Les pagines que son de codi i 
	les pagines de kernel (data i codi) les copiem directament del pare. Les pagines de data+stack
	son noves i les treurem del vector child_frames
  */

  //Aqui copiem les entrades de la taula de pagina de kernel i codi
  for(int i=0; i<PAG_LOG_INIT_DATA;i++){
	  //Fem que les pagines logiques del fill de kernel i codi s'assignin al mateix
	  //frame AKA pagina fisica que el pare. Llavors tenim dos pagines apuntant a la 
	  //mateixa localitzacio de memoria. Exito.
	  set_ss_pag(child_pt, i, get_frame(father_pt, i));
  }
  //Pillem una pagina aixi random que estigui 
  //Aqui inicialitzem les entrades de la taula de pagina de dades i stack i copiem les dades
  for(int i=0; i<NUM_PAG_DATA; i++){
	  //PAG_LOG_INIT_DATA es on esta la primera pagina de dades.
	  //Lo mismo que NUM_PAGE_KERNEL + NUM_PAGE_CODE 
	  set_ss_pag(child_pt, PAG_LOG_INIT_DATA+i, child_frames[i]); //Amb aixo hem inicialitzat la taula de pagines
	  //set_ss_pag(father_pt, )
  }

  return PID;
}

void sys_exit()
{  
        int p;
        /* Alliberem estructures de dades */
        struct task_struct * current_task_struct = current(); // PCB actual
        page_table_entry * current_page_table = get_PT(current_task_struct); // Taula de pagines del PCB
        
        for (p=PAG_LOG_INIT_DATA; p<TOTAL_PAGES; p++) {
            //Alliberem els 20 bits de pbase_addr, que conte numero de pag fisica (no @) de cada pag
            free_frame(current_page_table[p].bits.pbase_addr);
        }
        
        
        
        /* Falta scheduler per escollir nou proces a executar i fer el canvi de context */
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

