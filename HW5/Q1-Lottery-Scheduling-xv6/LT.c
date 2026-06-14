#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  printf("LT start\n");

  int tickets[4] = {10, 20, 40, 80};
  int i;

  for(i = 0; i < 4; i++){
    int pid = fork();
    if(pid < 0){
      printf("fork failed\n");
      exit(1);
    }
    if(pid == 0){
      int mypid = getpid();
      int mytickets = tickets[i];
      int r = settickets(mypid, mytickets);
      printf("child pid %d set tickets %d ret %d\n", mypid, mytickets, r);
      if(r < 0){
        exit(1);
      }

      volatile int counter = 0;
      int start = uptime();
      while(uptime() - start < 200){
        counter++;
      }

      printf("pid %d tickets %d counter %d\n", mypid, mytickets, counter);
      exit(0);
    }
  }

  for(i = 0; i < 4; i++){
    wait(0);
  }

  printf("LT done\n");
  exit(0);
}

