
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_set_new_field 452
#define __NR_get_new_field 453

int main(void)
{
    int val = 402105727;
    syscall(__NR_set_new_field, val);

    int ret = syscall(__NR_get_new_field);
    printf("new_field = %d\n", ret);
    return 0;
}
