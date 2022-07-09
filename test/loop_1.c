#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define SIZE 256

__attribute__((noinline))
int64_t index_rounder(int64_t index) {
  return index % SIZE;
}

int main(int argc, char** argv) {
  FILE* fd = fopen(argv[1], "r");
  if(!fd) return -1;

  char buf[SIZE];
  short check = -1;

  size_t ret = fread(buf, sizeof(char), SIZE, fd);
  fclose(fd);

  for(size_t i = 0; i < SIZE/4; i+=4) {
    if(buf[SIZE/15] < buf[index_rounder(i)] && buf[i+7] + buf[i+1] > buf[SIZE/2])
      check++;
  }

  if(check < 0)  { printf("CHECK UNDERFLOW\n"); return -2; }

  if(buf[7 + index_rounder(SIZE*4)] + buf[index_rounder(SIZE + SIZE/2 - SIZE)] == 'X') {
    printf("br1\n");
    if(buf[SIZE-SIZE/2] > 'Z') {
      printf("br2\n");
    } else {
      printf("vuln1\n");
      return 0;
    }
    if(buf[7] + buf[index_rounder((int64_t)buf[111])] == 'G') {
      printf("vuln2\n");
    } else {
      printf("br3\n");
      if(buf[111] + buf[222] < 'V') {
        printf("vuln3\n");
      }
    }
  }
  printf("br0\n");
  return 0;
}
