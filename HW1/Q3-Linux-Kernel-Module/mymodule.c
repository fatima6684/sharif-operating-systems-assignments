#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple test kernel module");

static int __init mymodule_init(void) {
    printk(KERN_INFO "mymodule: loaded successfully!\n");
    return 0;
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "mymodule: unloaded successfully!\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
