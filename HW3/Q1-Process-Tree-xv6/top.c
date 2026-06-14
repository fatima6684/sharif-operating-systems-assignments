#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char *states[] = {
  "UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"
};
void print_tree(struct process_data *procs, int n, int parent, int depth) {
  for(int i = 0; i < n; i++) {
    if(procs[i].parent_id == parent) {
      for(int j = 0; j < depth; j++) printf("  ");


        printf("├─ pid=%d (%s) [%s]\n", procs[i].pid, procs[i].name, states[procs[i].state]);

      print_tree(procs, n, procs[i].pid, depth + 1);
    }
  }
}

int main(int argc, char *argv[])
{
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

  if(argc > 1 && (!strcmp(argv[1], "-tree") || !strcmp(argv[1], "-t"))) {
    printf("Process Tree:\n");
    print_tree(all, count, 0, 0);
  } else {
    printf("PID\tPPID\tSIZE\tSTATE\tNAME\t\tSTATUS\n");
    for(int i = 0; i < count; i++) {

      char *status = (all[i].state != 1) ? "USED" : "UNUSED";
      printf("%d\t%d\t%ld\t%s\t%s\t%s\n", all[i].pid, all[i].parent_id,
             all[i].head_size, states[all[i].state], all[i].name, status);
    }
  }

  exit(0);
}
