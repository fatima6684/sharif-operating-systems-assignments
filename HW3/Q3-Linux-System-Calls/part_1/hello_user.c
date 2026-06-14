#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

#define __NR_hello_syscall 451

int main(void) {
	char buf[128] = {0};
	long ret = syscall(__NR_hello_syscall, buf);

	if (ret >= 0) {
		printf("kernel says: %s\n", buf);
	} else {
		perror("syscall failed :(\n");
	}

	return 0;
}
