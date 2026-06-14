#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

struct syscall_times {
    u64 enter_time;
    u64 exit_time;
};

SYSCALL_DEFINE1(get_syscall_times, struct syscall_times __user *, user_buf)
{
    struct syscall_times times;

    times.enter_time = current->syscall_enter_time;
    times.exit_time  = current->syscall_exit_time;

    if (copy_to_user(user_buf, &times, sizeof(times)))
        return -EFAULT;

    // Reset for next measurement
    current->syscall_enter_time = 0;
    current->syscall_exit_time  = 0;

    return 0;
}
