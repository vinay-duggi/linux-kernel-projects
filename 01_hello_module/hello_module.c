/*
 * hello_module.c - Simple Linux Kernel Module
 *
 * Demonstrates the basic structure of a kernel module:
 * - Module metadata (license, author, description)
 * - init function - called when module loaded (insmod)
 * - exit function - called when module removed (rmmod)
 * - printk - kernel equivalent of printf
 *   output goes to kernel ring buffer (dmesg)
 *
 * This is the "Hello World" of kernel programming
 */

 #include <linux/module.h> /* MODULE_LICENSE, module_init, module_exit */
 #include <linux/kernel.h> /* printk, KERN_INFO */
 #include <linux/init.h> /* __init__, __exit macros */
 #include <linux/utsname.h>



/* Module Metadata */
MODULE_LICENSE("GPL"); /* must be GPL for kernel APIs */
MODULE_AUTHOR("Vinay Duggineni");
MODULE_DESCRIPTION(" Hello World Linux Kernel Module ");
MODULE_VERSION("1.0");

/* Init function */
/*
 * __init - tells kernel this function only needed at load time
 *          memory freed after module initialization completes
 *          optimization - saves kernel memory
 */

static int __init hello_init(void)
{
    /*
     * printk - kernel's logging function
     * NOT printf - no stdout in kernel space
     * Output goes to kernel ring buffer
     * View with: dmesg | tail
     *
     * Log levels:
     * KERN_EMERG   - system unusable
     * KERN_ALERT   - action must be taken immediately
     * KERN_CRIT    - critical conditions
     * KERN_ERR     - error conditions
     * KERN_WARNING - warning conditions
     * KERN_NOTICE  - normal but significant
     * KERN_INFO    - informational
     * KERN_DEBUG   - debug-level messages
     */
    
     printk(KERN_INFO "hello_module: loaded successfully\n");
     printk(KERN_INFO "hello_module: kernel version %s\n", utsname() -> release);
    
     /*
     * Return 0 = success
     * Non-zero = failure (module won't load)
     */
    
    return 0;
}


/* Exit function */
/*
 * __exit - tells kernel this only needed at unload time
 *          ignored for built-in modules (not loadable)
 */

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: unloaded\n");
    /* void return - nothing to clean up in hello world */
}


/* register init and exit */
/*
 * These macros tell the kernel which functions to call
 * module_init → called by insmod
 * module_exit → called by rmmod
 */

module_init(hello_init);
module_exit(hello_exit);
