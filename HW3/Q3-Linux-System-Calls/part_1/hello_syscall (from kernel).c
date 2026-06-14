#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/errno.h>

SYSCALL_DEFINE0(hello_syscall)
{
    struct file *file;
    loff_t pos = 0;
    const char msg[] =
        "This is the second version of the syscall.\n"
        "Saying hi directly to the user space via stdout!\n";
    size_t len = sizeof(msg) - 1;
    ssize_t ret;

    // Get file pointer for stdout (fd = 1)
    file = fget(1);
    if (!file)
        return -EBADF;

    // Write to stdout directly (kernel 5.x/6.x safe)
    ret = kernel_write(file, msg, len, &pos);

    // Release the file pointer
    fput(file);

    return ret;
}
