#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_hello_syscall 451

int main() {
    syscall(__NR_hello_syscall);
    return 0;
}
