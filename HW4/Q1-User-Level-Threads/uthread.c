#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"

struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(struct context*, struct context*);

void
thread_init(void)
{
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void thread_schedule(void)
{
  int start = current_thread - all_thread; // index فعلی
  for(int i=1; i<=MAX_THREAD; i++){
    int idx = (start + i) % MAX_THREAD; // round-robin
    if(all_thread[idx].state == RUNNABLE){
      struct thread *prev = current_thread;
      current_thread = &all_thread[idx];
      current_thread->state = RUNNING;
      thread_switch(&prev->context, &current_thread->context);
      return;
    }
  }


  printf("thread_schedule: no runnable threads\n");
  exit(0);
}



void
thread_create(void (*func)())
{
  for(int i = 0; i < MAX_THREAD; i++) {
    if(all_thread[i].state == FREE) {
      struct thread *t = &all_thread[i];
      t->state = RUNNABLE;

      // stack pointer به بالای stack اشاره کند
      uint64 sp = (uint64)t->stack + STACK_SIZE;

      // context اولیه
      t->context.ra = (uint64)func; // وقتی switch کردیم، func اجرا شود
      t->context.sp = sp;

      return;
    }
  }
  printf("No free thread!\n");
}

void thread_yield(void)
{
  if(current_thread->state == RUNNING)
    current_thread->state = RUNNABLE;
  thread_schedule();
}



volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void thread_a(void) {
  a_started = 1;
  for(a_n = 0; a_n < 100; a_n++) {
    printf("thread_a %d\n", a_n);
    thread_yield();
  }
  printf("thread_a : exit after 100\n");
  current_thread->state = FREE;
  thread_schedule();
}

void thread_b(void) {
  b_started = 1;
  for(b_n = 0; b_n < 100; b_n++) {
    printf("thread_b %d\n", b_n);
    thread_yield();
  }
  printf("thread_b : exit after 100\n");
  current_thread->state = FREE;
  thread_schedule();
}

void thread_c(void) {
  c_started = 1;
  for(c_n = 0; c_n < 100; c_n++) {
    printf("thread_c %d\n", c_n);
    thread_yield();
  }
  printf("thread_c : exit after 100\n");
  current_thread->state = FREE;
  thread_schedule();
}

int
main(int argc, char **argv[])
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  thread_schedule();
  exit(0);
}
