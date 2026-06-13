#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int *a;
  int *b;
  int i;


  printf("--calloc test--\n");
  a = (int*)calloc(5, sizeof(int));

  for(i = 0; i < 5; i++)
    printf("a[%d]= %d\n", i, a[i]);



  printf("\n--realloc (expand) test--\n");
  printf("increase memmory\n");
  a = realloc(a, 10 * sizeof(int));

  for(i = 5; i < 10; i++)
    a[i] = i * 7;

  for(i = 0; i < 10; i++)
    printf("a[%d]= %d\n", i, a[i]);

  printf("decrease memmory\n");
  a = realloc(a, 2 * sizeof(int));


  for(i = 0; i < 2; i++)
    printf("a[%d]= %d\n", i, a[i]);



  printf("\n--realloc(ptr, 0) test--\n");
  b = a;
  a = realloc(a, 0);

  if(a == 0)
    printf("realloc(ptr, 0) returned NULL\n");

  a = malloc(10 * sizeof(int));
  if(a == b)
    printf("free and refuse memmory\n");
  else
    printf("memmory freed and address changed\n");

  free(a);



  printf("\n--(NULL, size) test--\n");
  a = realloc(0, 4 * sizeof(int));

  if(a != 0)
    printf("1-realloc(NULL, size) behaves like malloc\n");

  printf("2-Initial values with undefined velue so behaves like malloc) : \n");
  for(i = 0; i < 4; i++)
    printf("a[%d]= %d\n", i, a[i]);

  printf("3-values after manual initialization(i+2) : \n");
  for(i = 0; i < 4; i++)
    a[i] = i + 2;

  for(i = 0; i < 4; i++)
    printf("a[%d]= %d\n", i, a[i]);


  free(a);
  exit(0);
}
