#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE1(hello_syscall, char __user *, buf)
{
	const char msg[] = "This is the second version of the syscall. Saying hi directly to the user space!";
	long len = sizeof(msg);

	if (copy_to_user(buf, msg, len))
		return -EFAULT;

	return len;
}

