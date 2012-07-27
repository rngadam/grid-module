#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argn, char* argv[])
{
  int sysmem_fd;
  char* data;
  int length = 4096;
  int i;
  unsigned int* reg;
  if(argn < 2) {
	  printf("filename required\n");
	  return 1;
  }
  sysmem_fd = open(argv[1], O_RDWR);
  data = mmap(
	  0,  // starting address
	  length, // length
	  PROT_READ | PROT_WRITE,  //prot
	  MAP_SHARED, //flags
	  sysmem_fd, //filedescriptor
	  0x0 // offset
	  );
  close(sysmem_fd);
  printf("current value: %d\n", (int)data[512]);
  reg = (int*) &data[512];
  //*reg =  0x03030300;
  *reg = 0;
  /*
  for(i=0;i<length;i++) {
	  printf("%02x ", data[i]);
  }*/
  munmap(data, length);

  return 0;
}
