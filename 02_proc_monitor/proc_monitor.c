/*===KERNEL HEADERS=== */
/* We cannot use standard user-space libraries (like <stdio.h>)
    - We must use the kernel's own internal header files */
    
#include <linux/module.h>   // Required by all loadable modules
#include <linux/kernel.h>   // Required for kernel macros like KERN_INFO and printk()
#include <linux/init.h>     // Required for the __init and __exit macros
#include <linux/proc_fs.h>  // Required for creating virtual files in /proc
#include <linux/seq_file.h> // The Sequential File API (safely buffers large outputs)
#include <linux/mm.h>       // Memory Management (si_meminfo resides here)
#include <linux/sched.h>    // Scheduling data (sometimes needed for process info)
#include <linux/utsname.h>  // System name structure (gets us the kernel version)
#include <linux/cpumask.h>  // CPU configuration (gets us the online CPU count)
#include <linux/jiffies.h>  // The kernel's internal timer tick counter

/* === MODULE METADATA===*/
/* The kernel requires a compatible license (like GPL) to allow module to access advcanced kernel functions */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinay Duggineni");
MODULE_DESCRIPTION("Custom /proc monitor exposing system metrics");
MODULE_VERSION("1.0");

/* Name of the virtual file we will create: /proc/sysmonitor */
#define PROC_NAME "sysmonitor"

/* ===READ/SHOW FUNCTION=== */
/* Function executes everytime we run 'cat /proc/sysmonitor'.
    It gets the real-time data and writes it into the kernel's 'seq_file' buffer (m) */

static int sysmonitor_show(struct seq_file *m, void *v)
{
    // Local struct to hold raw memory numbers. It lives on the tiny kernel stack.
    struct sysinfo si;
    unsigned long uptime_seconds;
    unsigned long uptime_minutes;
    unsigned long uptime_hours;

    // we pass the memory address of our local struct to the kernel.
    // The kernel will fill this struct with the current RAM usage data.
    si_meminfo(&si);
    
    // jiffies is a global kernel variable counting timer ticks since booting.
    // We convert it to milliseconds, then to seconds for human-readability.
    uptime_seconds = jiffies_to_msecs(jiffies) / 1000;
    uptime_hours = uptime_seconds / 3600;               // Extract total hours
    uptime_minutes = (uptime_seconds % 3600) / 60;      // Extract leftover minutes
    uptime_seconds = uptime_seconds % 60;               // Extract leftover seconds

    //seq_printf safely formats text and pushes it to the 'm' buffer.
    seq_printf(m, "kernel_version: %s\n", utsname() -> release);                                // utsname()->release to get the OS version string (e.g.6.17.0-14-generic)
    seq_printf(m, "uptime: %lu:%02lu:%02lu\n", uptime_hours, uptime_minutes, uptime_seconds);   // %lu = long unsigned integer, %02lu = pad with a leading zero if single digit
    seq_printf(m, "cpu_count: %d\n", num_online_cpus());                                        // counts how many CPU cores are currently powered on
    seq_printf(m, "mem_total_kb: %lu\n", si.totalram *(si.mem_unit / 1024));                    // Memory Math: si.totalram counts "pages" of memory, not bytes.
    seq_printf(m, "mem_free_kb: %lu\n", si.freeram * (si.mem_unit / 1023));                     // We multiply by si.mem_unit (bytes per page) and divide by 1024 to get Kilobytes.
    seq_printf(m, "mem_available_kb: %lu\n", (si.freeram + si.bufferram) * (si.mem_unit / 1024)); // Available memory includes free RAM plus buffer/cache RAM that can be reclaimed instantly.
    seq_printf(m, "mem_used_kb: %lu\n", (si.totalram - si.freeram) * (si.mem_unit / 1024));      // Used memory is Total - Free.
    seq_printf(m, "swap_total_kb: %lu\n", si.totalswap * (si.mem_unit / 1024));
    seq_printf(m, "swap_free_kb: %lu\n", si.freeswap * (si.mem_unit / 1024));
    return 0; // read was completely successful
}


/* ===OPEN FUNCTION=== */
/* When we the user open the file, this attaches the active session ('file') to our data gen function ('sysmonitor_show') */
static int sysmonitor_open(struct inode *inode, struct file *file)
{
    return single_open(file, sysmonitor_show, NULL); // single_open is a helper function saying: "This file is simple."
                                                        // just run sysmonitor_show to generate all the data at once.
}

/* ===FILE OPERATIONS=== */
/* We have this struct act as a lookup table for the VFS.
    It maps standard Linux file ops. to our custom functions. */
static const struct proc_ops sysmonitor_ops = {
    .proc_open = sysmonitor_open,   // When opening, run our setup
    .proc_read = seq_read,          // seq_file API handles the actual reading
    .proc_lseek = seq_lseek,        // seq_file API handles the seeking (cursormovement by using offset)
    .proc_release = single_release, // seq_file API handles the clean up memory when closed
};

/* ===INITIALIZATION FUNCTION (Entry Point)===*/
/* The __init macro tells the kernel to delete this function from RAM
    as soon as it finishes running, saving memory. */
static int __init sysmonitor_init(void)
{
    // Creates the actual file at /proc/sysmonitor
    // 0444 sets permissions to Read-Only for Owner, Group, and Others.
    // We pass our sysmonitor_ops struct address to pipe everything.
    proc_create(PROC_NAME, 0444, NULL, &sysmonitor_ops);
    printk(KERN_INFO "sysmonitor: created /proc/%s\n", PROC_NAME); // printk logs a message to the kernel ring buffer (viewable with dmesg).
    return 0; // Success
}

/* ===CLEANUP FUNCTION (Exit Point)=== */
/* The __exit macro marks this code to only run when the module is being removed (rmmod). */
static void __exit sysmonitor_exit(void)
{
    // WE MUST CLEAN UP. If we don't remove the /proc entry, 
    // reading it after the module is unloaded will crash the kernel.
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "sysmonitor: removed /proc/%s\n", PROC_NAME);
}

/* ===MODULE REGISTRATION=== */
/* Tell the kernel which functions to call when loading (insmod) and unloading (rmmod) */
module_init(sysmonitor_init);
module_exit(sysmonitor_exit);