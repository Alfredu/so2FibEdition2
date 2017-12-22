#include <libc.h>

char buff[24];

int pid;

int add(int par1, int par2) {
	/*asm("movl 8(%ebp), %eax;"
		"addl 12(%ebp), %eax;"
		);*/

	int result;
	asm("movl 8(%%ebp), %0;"
		"addl 12(%%ebp), %0;"
		:"=r"(result)
		);
	return result;
}

long inner (long n){
	int i;
	long suma;
	suma = 0;
	for(i=0; i<n; i++) suma = suma+i;
	return suma;
}

long outer (long n)
{
	int i;
	long acum;
	acum = 0;
	for (i = 0; i < n; ++i)
	{
		acum = acum + inner(i);
	}
	return acum;
}

int cont = 0;
void printeja(){
	sem_wait(0);
	cont++;
	char buff[128];
	itoa(cont, buff);
	write(1, buff, strlen(buff));
	write(1, "\n", 1);
	sem_signal(0);
	exit(0);

}
int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	// runjp_rank();
	char buffer[128];
	read(0, buffer, 1);
	buffer[1] = '\n';
	write(1, buffer, strlen(buffer));
	 while(1){
	 }
 	
 	return 0;
}
