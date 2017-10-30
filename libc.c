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
  asm("movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl %3, %%edx;"
      "movl $4, %%eax;"
      "int $0x80;"
      "movl %%eax, %0;"
      : "=r" (ret)
      : "r" (fd), "m" (buffer), "r" (size));
  if (ret<0) {
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

void perror(){
  char * buffer = sys_errlist[errno];
  write(1, buffer, strlen(buffer));
}