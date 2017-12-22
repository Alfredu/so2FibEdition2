/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>
int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int write(int fd, char * buffer, int size){
  int ret;
  /*
  asm("movl 8(%ebp), %ebx;"
      "movl 12(%ebp), %ecx;"
      "movl 16(%ebp), %edx;"
      "movl $4, %eax;"
      "int $0x80;");*/

  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl %3, %%edx;"
      "movl $4, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (fd), "m" (buffer), "r" (size));
  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret;
}

int read(int fd, char *buffer, int count){
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl %3, %%edx;"
      "movl $5, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (fd), "m" (buffer), "r" (count));
  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret;
}
int get_stats(int pid, struct stats *st){
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl $35, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (pid), "m"(st));

  if(ret<0){
    errno=-ret;
    return -1;
  }

  return ret;
}

int gettime(){
  int ret;
  
  asm("movl $10, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      : "=r" (ret)
      );
  if(ret<0){
    errno=-ret;
    return -1;
  }
  return ret;
}

int getpid(){
  int ret;
  asm("movl $20, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      : "=r" (ret)
    );
  if(ret<0){
    errno=-ret;
    return -1;
  }
  return ret;
}

int fork(){
  int ret;
  asm("movl $2, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      : "=r" (ret));
    
  if (ret<0){
    errno=-ret;
    return -1;
  }
  return ret;
}

void exit() {
   
   asm("movl $1, %eax;"
      "int $0x80;"
    );
   
}

void perror(){
  char * buffer = sys_errlist[errno];
  write(1, buffer, strlen(buffer));
}

int clone(void (*function) (void), void *stack) {
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl $19, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "g" (function), "g" (stack));
  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret;
}

int sem_init (int n_sem, unsigned int value) {
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl $21, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (n_sem), "m" (value));

  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret; 
}

int sem_wait (int n_sem) {
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl $22, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (n_sem));

  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret; 
}

int sem_signal (int n_sem) {
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl $23, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (n_sem));
      
  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret; 
}


int sem_destroy (int n_sem) {
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl $24, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (n_sem));
      
  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret; 
}

int *sbrk(int increment) {
  int ret;
  asm("pushl %%ebx;"
      "movl %1, %%ebx;"
      "movl $13, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      "popl %%ebx;"
      : "=r" (ret)
      : "r" (increment));
      
  if (ret<0) {
    errno=-ret;
    return -1;
  }
  return ret; 
}



