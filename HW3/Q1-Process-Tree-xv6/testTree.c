#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void busy_work() {
  volatile long i;
  for(i = 0; i < 500000000; i++);
}

void print_tree(struct process_data *procs, int n, int parent, int depth) {
  char *states[] = { "UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE" };
  for(int i = 0; i < n; i++) {
    if(procs[i].parent_id == parent) {
      for(int j = 0; j < depth; j++) printf("  ");
      char *status;
      if(procs[i].state != 1)       status = "USED";
      else                           status = "UNUSED";

      printf("├─ pid=%d (%s) [%s] {%s}\n", procs[i].pid, procs[i].name,
             states[procs[i].state], status);
      print_tree(procs, n, procs[i].pid, depth + 1);
    }
  }
}

int main(int argc, char *argv[]) {
  int i;

  printf("Parent PID: %d\n", getpid());

  //Test case 1: Zombie
  int zombie_pid = fork();
  if(zombie_pid == 0) {
    printf("Zombie child PID: %d, Parent: %d (will exit and become zombie)\n", getpid(), getppid());
    exit(0);
  }

  //Test case 2: USED
  int used_pid = fork();
  if(used_pid == 0) {
    printf("USED child PID: %d, Parent: %d (simulated USED state)\n", getpid(), getppid());
    sleep(3);
    exit(0);
  }

  // Test case 3: Running child ---
  int running_pid = fork();
  if(running_pid == 0) {
    printf("RUNNING child PID: %d, Parent: %d\n", getpid(), getppid());
    busy_work();
    exit(0);
  }

  // Test case 4: Sleeping
  int sleeping_pid = fork();
  if(sleeping_pid == 0) {
    printf("SLEEPING child PID: %d, Parent: %d\n", getpid(), getppid());
    sleep(5);
    exit(0);
  }

  sleep(2);

  struct process_data all[64];
  int count = 0;
  int pid = 0;
  struct process_data p;

  while(1) {
    int r = next_process(pid, &p);
    if(r == 0)
      break;
    all[count++] = p;
    pid = p.pid;
  }

  printf("\nPID\tPPID\tSIZE\tSTATE\tNAME\tSTATUS\n");
  char *states[] = { "UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE" };
  for(i = 0; i < count; i++) {
    char *status;
    if(all[i].state != 1)       status = "USED";
    else                           status = "UNUSED";

    printf("%d\t%d\t%ld\t%s\t%s\t%s\n", all[i].pid, all[i].parent_id,
           all[i].head_size, states[all[i].state], all[i].name, status);
  }

  printf("\nProcess Tree:\n");
  print_tree(all, count, 0, 0);
  for(i = 0; i < 4; i++)
    wait(0);

  exit(0);
}
