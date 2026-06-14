
struct spinlock {
  uint locked;       

  char *name;        
  struct cpu *cpu;   
#ifdef LAB_LOCK
  int nts;
  int n;
#endif
};

#ifdef LAB_LOCK
struct rwspinlock {
  struct spinlock lk;
  int readers;
  int writer;
  int waiting_writers;
};
#endif
