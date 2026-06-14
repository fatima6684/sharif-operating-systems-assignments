#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/sched.h>


SYSCALL_DEFINE1(set_new_field, int, value) {
	struct task_struct *task = current;
	task->new_field = value;
	printk(KERN_INFO "set new_field = %d for PID %d\n", task->new_field, task->pid);
	return 0;
}

SYSCALL_DEFINE0(get_new_field) {
	struct task_struct *task = current;
	printk(KERN_INFO "Get new_field = %d for PID = %d\n", task->new_field, task->pid);
	return task->new_field;
}
