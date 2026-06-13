// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

#define NPHYPAGES ((PHYSTOP - KERNBASE) / PGSIZE)
static int refcnt[NPHYPAGES];

static inline int
pa2idx(uint64 pa)
{
  return (pa - KERNBASE) / PGSIZE;
}

void
kincref(void *pa)
{
  uint64 x = (uint64)pa;
  if(x < KERNBASE || x >= PHYSTOP || (x % PGSIZE) != 0)
    panic("kincref");

  acquire(&kmem.lock);
  refcnt[pa2idx(x)]++;
  release(&kmem.lock);
}

int
krefcount(void *pa)
{
  uint64 x = (uint64)pa;
  if(x < KERNBASE || x >= PHYSTOP || (x % PGSIZE) != 0)
    return 0;

  acquire(&kmem.lock);
  int r = refcnt[pa2idx(x)];
  release(&kmem.lock);
  return r;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    // set initial refcount so kfree() will drop it to 0 and really free.
    acquire(&kmem.lock);
    refcnt[pa2idx((uint64)p)] = 1;
    release(&kmem.lock);
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa.
void
kfree(void *pa)
{
  struct run *r;
  uint64 x = (uint64)pa;

  if((x % PGSIZE) != 0 || (char*)pa < end || x >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);

  int idx = pa2idx(x);
  if(refcnt[idx] < 1)
    panic("kfree: refcnt");

  refcnt[idx]--;
  if(refcnt[idx] > 0){
    // still referenced somewhere
    release(&kmem.lock);
    return;
  }

  // now really free
  release(&kmem.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}


void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    refcnt[pa2idx((uint64)r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE);
  return (void*)r;
}
