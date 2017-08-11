#include <stdio.h>
#include <unistd.h>
#include "base/memory/ptr_util.h"


void printVmStats()  {
  FILE* f = fopen("/proc/self/statm", "r");
  unsigned long vmSize = 0, vmRss = 0;
  fscanf(f, "%lu %lu", &vmSize, &vmRss);
  fclose(f);
  printf("VirtSize=%lu KB, RSS=%lu KB\n", vmSize * 4, vmRss * 4);
}

int main() {
  printf("just started\n"); fflush(stdout);
  printVmStats();
  getchar();

  const unsigned long kAllocSize = 65 * 1024 * 1024;
  void* buffer = malloc(kAllocSize);
  memset(buffer, 1, kAllocSize);
  volatile char* x = static_cast<volatile char*>(buffer);
  if(x[0] + x[kAllocSize - 1] != 2)
    return 1;

  printf("malloc done, press a key to .reset()\n"); fflush(stdout);
  printVmStats();
  getchar();

  free(buffer);
  printf("reset done, press a key to exit()\n"); fflush(stdout);
  printVmStats();
  getchar();

  return 0;
}
