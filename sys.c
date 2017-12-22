/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <list.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define BUFFER_SIZE 256
extern int zeos_ticks;

void switch_user_to_system(){
	update_stats(&(current()->task_stats.user_ticks), &(current()->task_stats.elapsed_total_ticks));
}

void switch_system_to_user(){
	update_stats(&(current()->task_stats.system_ticks), &(current()->task_stats.elapsed_total_ticks));
}

int check_fd_write(int fd, int permissions)
{
  if (fd!=1) return -EBADF; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  return 0;
}

int check_fd_read(int fd, int permissions)
{
	if (fd!=0) return -EBADF; /*EBADF*/
  	if (permissions!=LECTURA) return -EACCES; /*EACCES*/
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

int ret_from_fork(){
	return 0;
}

int sys_fork()
{
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

	/*Ara procedirem a omplir la taula de pagines del fill. Els frames que son de codi i 
	les pagines de kernel (data i codi) les copiem directament del pare. Els frames de data+stack
	son noves i les treurem del vector child_frames
	*/
	//Aqui copiem les entrades de la taula de pagina de kernel i codi
	for(int i=0; i<PAG_LOG_INIT_DATA;i++){
		//Fem que les pagines logiques del fill de kernel i codi s'assignin al mateix
		//frame AKA pagina fisica que el pare. Llavors tenim dos pagines apuntant a la 
		//mateixa localitzacio de memoria. Exito.
		set_ss_pag(child_pt, i, get_frame(father_pt, i));
	}

	/*Pillem la primera pagina que estigui lliure del papi.*/
	unsigned int tmp_logpage=-1;
	for(int i=PAG_LOG_INIT_DATA+NUM_PAG_DATA;i<TOTAL_PAGES;i++){
		if(!father_pt[i].entry){
			tmp_logpage = i;
			break;
		}
	}
	if(tmp_logpage<0){
		//No hi havia cap pagina lliure. S'hauria de alliberar tot lo que hem alocatat fins ara.
		return -EAGAIN;
	}

	//Aqui inicialitzem les entrades de la taula de pagina de dades i stack i copiem les dades
	for(int i=0; i<NUM_PAG_DATA; i++){
		//PAG_LOG_INIT_DATA es on esta la primera pagina de dades.
		//Lo mismo que NUM_PAGE_KERNEL + NUM_PAGE_CODE .
		set_ss_pag(child_pt, PAG_LOG_INIT_DATA+i, child_frames[i]); //Associem pagina amb frame.
		set_ss_pag(father_pt, tmp_logpage, child_frames[i]); //Associem la pagina temporal amb el mateix frame.
		/*Ara hem de copiar les dades del pare al fill. Ara ja podem perque tenim
		una pagina del pare, tmp_logpage associada a aquell frame. Copiarem allà i després borrarem
		la entrada de la taula del pare.*/ 
		//Multipliquem per page_size per tenir la ADREÇA logica. No volem la pagina, volem adreça.
		copy_data((void *)((PAG_LOG_INIT_DATA+i)*PAGE_SIZE),(void *)((tmp_logpage)*PAGE_SIZE), PAGE_SIZE);
		//Desfem la traducció de tmp_logpage al child_frames[i]
		del_ss_pag(father_pt, tmp_logpage);
		//flush del tlb per asegurarnos que no hi han traduccions als frames del fill
		set_cr3(get_DIR(&father_task_union->task));
	}
	//set_cr3(get_DIR(&father_task_union->task));
	child_task_union->task.PID = getNextPid();
	father_pt[tmp_logpage].entry = 0;
	child_task_union->task.kernel_esp = &child_task_union->stack[KERNEL_STACK_SIZE-19];
	child_task_union->stack[KERNEL_STACK_SIZE-19] = 0;
	child_task_union->stack[KERNEL_STACK_SIZE-18] = &ret_from_fork;

	//resetegem stats del fill
	init_task_stats(&child_task_union->task.task_stats);
	list_add_tail(&child_task_union->task.list, &readyqueue);
	return child_task_union->task.PID;
}

void sys_exit()
{  
	/* Alliberem estructures de dades */
	struct task_struct * current_task_struct = current(); // PCB actual

	int dirPos = get_DIR_pos(get_DIR(current_task_struct));
	dir_references[dirPos]--;
	if(dir_references == 0){
		free_user_pages(current_task_struct);
	}
	current_task_struct->PID = -1;
	list_add_tail(&current_task_struct->list, &freequeue);
	sched_next_rr();
}

int sys_write(int fd, char * buffer, int size) {
	int ret = check_fd_write(fd, ESCRIPTURA);
	if (ret < 0) return ret;
	if (!buffer) return -EFAULT;
	if (size<0) return -EINVAL;

	char pointer[BUFFER_SIZE];
	if (!access_ok(VERIFY_READ, buffer, size))
		return -EFAULT;
		
	while(size > BUFFER_SIZE && ret){ //in case it wrote 0 bytes. Although it always writes "size".
		copy_from_user(buffer, pointer, BUFFER_SIZE);
		ret = sys_write_console(pointer, BUFFER_SIZE);
		size = size - BUFFER_SIZE;
	}
	copy_from_user(buffer, pointer, size);
	ret = sys_write_console(pointer, size);

	return ret;
}
//Read independent
int sys_read(int fd, char *buffer, int count)
{
	int ret = check_fd_read(fd, LECTURA);
	if(ret<0) return ret;
	if (!buffer) return -EFAULT;
	if (count<0) return -EINVAL;
	if (!access_ok(VERIFY_WRITE, buffer, count))
		return -EFAULT;
	ret = sys_read_keyboard(buffer, count);
	return ret;
}

int sys_gettime() 
{
	return zeos_ticks;
}

int sys_get_stats(int pid, struct stats *st){
	
	if(current()->PID == pid){
		copy_to_user(&(current()->task_stats), st, sizeof(struct stats));
		return 0;
	}
	else{
		for(int i=0;i<NR_TASKS;i++){
			if(task[i].task.PID == pid){
				copy_to_user(&(task[i].task.task_stats), st, sizeof(struct stats));
				return 0;
			}
		}
	
	}
	return -1;
}

int sys_clone(void (*function) (void), void *stack) {
	if(list_empty(&freequeue)) return -ENOMEM;
	//TODO Check access
	if (!access_ok(VERIFY_WRITE, stack,4) || !access_ok(VERIFY_READ, function, 4)) return -EFAULT;
	struct list_head * child_list_head = list_first(&freequeue);
	list_del(child_list_head);
	union task_union *father_task_union = (union task_union *) current();
	union task_union *child_task_union = (union task_union *)list_head_to_task_struct(child_list_head);
	
	int dirPos = get_DIR_pos(get_DIR(&father_task_union->task));
	dir_references[dirPos]++;	

	copy_data(father_task_union, child_task_union, sizeof(union task_union));

	child_task_union->task.PID = getNextPid();
	//Ara fem el prank enllaç dinamic com al fork
	child_task_union->task.kernel_esp = &child_task_union->stack[KERNEL_STACK_SIZE-19];
	child_task_union->stack[KERNEL_STACK_SIZE-19] = 0;
	child_task_union->stack[KERNEL_STACK_SIZE-18] = &ret_from_fork;
	/*Modifiquem el context guardat pel SAVE_ALL i canviem la @ret
	per function i fem que apunti a la stack que ens dona el user*/
	child_task_union->stack[KERNEL_STACK_SIZE-5] = function;
	child_task_union->stack[KERNEL_STACK_SIZE-2] = stack;
	init_task_stats(&child_task_union->task.task_stats);
	list_add_tail(&child_task_union->task.list, &readyqueue);
	return child_task_union->task.PID;
}

int sys_sem_init(int n_sem, unsigned int value) {
	if(n_sem < 0 || n_sem >= NR_SEMAPHORES) return -ENAVAIL;
	if(semaphores[n_sem].owner!=-1){
		return -ENAVAIL;
	}
	semaphores[n_sem].owner = current()->PID;
	semaphores[n_sem].counter = value;
	INIT_LIST_HEAD(&semaphores[n_sem].queue);
	return 0;
}



int sys_sem_wait(int n_sem) {
	if(n_sem < 0 || n_sem > NR_SEMAPHORES) return -ENAVAIL;

	if (semaphores[n_sem].counter > 0) {
		semaphores[n_sem].counter--;
		return 0;
	} else {
		// TODO distingir
		update_process_state_rr(current(), &semaphores[n_sem].queue);
		sched_next_rr();
		if(semaphores[n_sem].owner != -1)
			return 0;
		else
			return -ENAVAIL;
	}
}

int sys_sem_signal(int n_sem) {
	if(n_sem < 0 || n_sem>NR_SEMAPHORES)
		return -ENAVAIL;
	if (list_empty(&semaphores[n_sem].queue)) {
		semaphores[n_sem].counter++;
	} else {
		struct task_struct *process_to_unlock;
		struct list_head *sem_list = list_first(&semaphores[n_sem].queue);
		list_del(sem_list);
		process_to_unlock = list_head_to_task_struct(list_first(sem_list));
		update_process_state_rr(process_to_unlock, &readyqueue);
	}
	return 0;
}

int sys_sem_destroy(int n_sem) {
	if(n_sem<0 || n_sem > NR_SEMAPHORES)
		return -ENAVAIL;
	if (current()->PID != semaphores[n_sem].owner) {
		return -ENAVAIL;
	} 
	else {
		semaphores[n_sem].owner = -1;
		while(!list_empty(&semaphores[n_sem].queue)){
			struct list_head *process_blocked = list_first(&semaphores[n_sem].queue);
			list_del(process_blocked);
			update_process_state_rr(list_head_to_task_struct(process_blocked), &readyqueue);
		}
		return 0;
	}
}

int sys_sbrk(int increment) {
	if (increment == 0) {
		//retorna pos
		return 0;
	} else {
		struct task_struct *current_ts = current();
		char *current_pb = (char *)current_ts->program_break;
		char *future_pb = (unsigned int)(current_pb+increment);
		int pages_needed = ((unsigned int)(future_pb-current_pb)/PAGE_SIZE);
		if( pages_needed > 0) {
			//Vol dir que necessitem mes pagines
			int new_frames[pages_needed];
			for(int i=0; i<pages_needed; i++){
				new_frames[i] = alloc_frame();
				if(new_frames[i] < 0){ //ERROR 
					//ALLIBERAR TOTS ELS FRAMES
					for(int j=0; j<i; j++){
						free_frame(new_frames[j]);
					}
					return -ENOMEM;
				}
			}
			page_table_entry *current_pt = get_PT(current_ts);
			int new_log_pages[pages_needed];
			int j=0;
			for(int i=PAG_LOG_INIT_DATA+NUM_PAG_DATA;i<TOTAL_PAGES || j<pages_needed;i++){
				if(current_pt[i].entry == 0){
					new_log_pages[j] = i;
					j++;
				}
			}
			if(j!=pages_needed){
				//No hi havia cap pagina lliure. S'hauria de alliberar tot lo que hem alocatat fins ara.
				return -EAGAIN;
			}

			for(int i=0; i<pages_needed; i++){
				
			}

		}
		
		
		
		
		
		
		else{
			//Augmentem progrma break
			current_ts->program_break = (void *)future_pb;
		}
	}
}
