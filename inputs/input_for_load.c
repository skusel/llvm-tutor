#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int* globalPtr = NULL;
int** globalPtrPtr = &globalPtr;

int getMax(int* array, unsigned n) {
  int max = 0;
  for(unsigned i = 0; i < n; i++) {
    if(array[i] > max) {
      max = array[i];
    }
  }
  return max;
}

int getMin(int* array, unsigned n) {
  int min = 1000;
  for(unsigned i = 0; i < n; i++) {
    if(array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

int main(int argc, char* argv[]) {
  unsigned n;

  printf("Please enter an array size: ");
  scanf("%u", &n);

  int* array = (int*)malloc(n * sizeof(int));

  srand(time(NULL));
  for(unsigned i = 0; i < n; i++) {
    array[i] = rand() % 1000;
  }

  int firstEl = *array;
  int lastEl = *(array + n - 1);

  printf("First: %d, last: %d, min: %d, max: %d\n", firstEl, lastEl, getMin(array, n), getMax(array, n));

  free(array);
  array = NULL;

  int* heapInt = (int*)malloc(sizeof(int));
  array += (int64_t)heapInt / sizeof(int);
  array = heapInt;
  *array = 1;

  free(array);
  array = *globalPtrPtr;

  int badFirstEl = 0;
  badFirstEl = *array;
  int ret = badFirstEl & 0;

  return ret;
}
