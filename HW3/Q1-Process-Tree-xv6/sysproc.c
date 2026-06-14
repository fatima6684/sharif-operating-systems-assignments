#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
extern struct proc proc[NPROC];


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


uint64
sys_top(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state != UNUSED){
      printf("%d\t%s\t%ld\n", p->pid, p->name, p->sz);
    }
  }

  return 0;
}


uint64
sys_next_process(void)
{
  int before_pid;
  uint64 proc_addr; // user pointer to struct process_data

  // گرفتن آرگومان‌ها از فضای کاربر
  argint(0, &before_pid);
  argaddr(1, &proc_addr);

  struct proc *p;
  struct proc *target = 0;

  // پیدا کردن کوچک‌ترین PID که بزرگ‌تر از before_pid باشد
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state != UNUSED && p->pid > before_pid) {
      if(target == 0 || p->pid < target->pid)
        target = p;
    }
  }

  if(target == 0) {
    return 0; // هیچ پردازه‌ای بزرگ‌تر از before_pid نیست
  }

  struct process_data pdata;
  pdata.pid = target->pid;
  pdata.parent_id = target->parent ? target->parent->pid : 0;
  pdata.head_size = target->sz;
  pdata.state = target->state;
  safestrcpy(pdata.name, target->name, sizeof(pdata.name));

  // کپی ساختار به فضای کاربر
  if(copyout(myproc()->pagetable, proc_addr, (char *)&pdata, sizeof(pdata)) < 0)
    return -1;

  return 1; // موفقیت‌آمیز
}

uint64
sys_sleep(void)
{
  int n;
  argint(0, &n);

  acquire(&tickslock);
  uint ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}
uint64
sys_getppid(void)
{
  struct proc *p = myproc();
  if(p->parent)
    return p->parent->pid;
  return 0;
}




