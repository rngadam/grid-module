#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argn, char* argv[])
{
  int fd;
  char* data;
  int length = 4096;
  int i;
  if(argn < 2) {
	  printf("filename required\n");
	  return 1;
  }
  fd = open(argv[1], O_RDWR);
  data = mmap(
	  0,  // starting address
	  length, // length
	  PROT_READ | PROT_WRITE,  //prot
	  MAP_SHARED, //flags
	  fd, //filedescriptor
	  0x0 // offset
	  );
  close(fd);
  printf("reading from %p\n", data);
  for(i=0;i<length;i++) {
	  printf("%02x ", data[i]);
  }
  munmap(data, length);

  return 0;
}
