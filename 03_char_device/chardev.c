/* ===KERNEL HEADERS=== */
#include <linux/module.h>   // Core header for loading/unloading modules into kernel
#include <linux/kernel.h>   // Contains types, macros, and functions like printk() 
#include <linux/init.h>     // Contains the __init and __exit macros
#include <linux/fs.h>       // File System definitions (alloc_chrdev_region, file_operations)
#include <linux/cdev.h>     // Character Device API (struct cdev, cdev_add, cdev_init)
#include <linux/uaccess.h>  // CRITICAL: Functions to cross the User/Kernel boundary (copy_to_user)
#include <linux/device.h>   // Device model (class_create, device_create for /dev node auto-creation)
#include <linux/ioctl.h>    // I/O Control macros (_IO, _IOR, _IOW) for custom hardware commands
#include <linux/slab.h>     // Kernel memory allocator (kmalloc, kfree)


/* ===MODULE METADATA=== */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinay Duggineni");
MODULE_DESCRIPTION("Virtual hardware accelerator character device driver");
MODULE_VERSION("1.0");


/* ===DEFINITIONS & MACROS=== */
#define DEVICE_NAME         "v_accelerator" // The name that will appear in /proc/devices
#define CLASS_NAME          "v_accel"       // The name of the sysfs class
#define BUF_SIZE            4096            // 4KB buffer (standard single page size)


/* ===IOCTL MAGIC NUMBERS=== */
/* We must keep ioctl numbers unique globally to prevent sending a command to wrong device.
    'A' is our "Magic Letter" acting as our driver's unique fingerprint. */
#define ACCEL_MAGIC         'A'
#define ACCEL_PROCESS       _IO(ACCEL_MAGIC, 1)     //_IO means "Command with no data transfer. 1&2 are just sequential command IDs."
#define ACCEL_RESET         _IO(ACCEL_MAGIC, 2)
#define ACCEL_GET_STATUS    _IOR(ACCEL_MAGIC, 3, int)   // _IOR means "Command that Reads data FROM the kernel TO the user".
#define ACCEL_GET_COUNT     _IOR(ACCEL_MAGIC, 4, int)   // We pass the data type (int) so the macro calculates the exact byte size automatically. 


/* ===GLOBAL KERNEL VARIABLES=== */
/* These variables define the state of our "virtual hardware" */
static dev_t dev_num;               // Holds the Major and Minor numbers assigned by the kernel
static struct cdev accel_cdev;      // The internal kernel structure representing our character device
static struct class *accel_class;   // Used to automatically tell 'udev' to create the /dev/ file
static char *device_buffer;         // Pointer to our heap-allocated memory (Kernel RAM)

/* State Machine Variables */
static int data_size = 0;           // How many bytes the user actually wrote
static int data_ready = 0;          // Boolean flag: 1 if data is processed and ready to read
static int op_count = 0;            // Lifetime counter of how many processing operations occurred


/* ===FILE OPERATIONS (VFS Callbacks)=== */
/* Called when we as a user space program runs 'open("/dev/accelerator", ...) */
static int accel_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "accelerator: device opened\n");
    return 0;
}

/* Called when the user space program runs 'close(fd)' */
static int accel_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "accelerator: device closed\n");
    return 0;
}

/* Called when the user runs 'write(fd, my_data, length)' */
static ssize_t accel_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    // Safety Check: Never allows the user to write past the buffer size.
    if (count > BUF_SIZE)
    {
        count = BUF_SIZE;
    }
    
    // Boundary Crossing: Safely pull data from User RAM into Kernel RAM.
    // copy_from_user(DESTINATION, SOURCE, SIZE). Returns the number of bytes that FAILED to copy.
    if (copy_from_user(device_buffer, buf, count))
    {
        return -EFAULT;
    }
    data_size = count; // Update the state machine.
    data_ready = 1;

    printk(KERN_INFO "accelerator: received %zu bytes\n", count);
    return count;     // Tell the user program exactly how many bytes we successfully wrote
}

/* Called when the user runs 'read(fd, my_buffer, length)' */
static ssize_t accel_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    // If no data has been written yet, return 0 (End of File / No Data)
    if (!data_ready)
    {
        return 0;
    }
    // Safety Check: Don't let the user read more data than we actually have stored.
    if (count > data_size)
    {
        count = data_size;
    }
    // Boundary Crossing: Safely push data from Kernel RAM to User RAM.
    if (copy_to_user(buf, device_buffer, count))
    {
        return -EFAULT;
    }
    data_ready = 0; // Reset the state so the user can't read the same data twice without writing new data.
    printk(KERN_INFO "accelerator: sent %zu bytes\n", count);
    return count; 
}


/* Called when the user runs 'ioctl(fd, COMMAND, ARGUMENT)' */
static long accel_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    // switch-case to route based on magic numbers defined at the top
    switch (cmd) {
        case ACCEL_PROCESS:
        {
            if (!data_ready)
            {
                return -ENODATA; // Error: No data to process
            }
            // This is the "Hardware Acceleration". 
            // We do a bitwise XOR against 0xFF to invert every single bit.
            for (int i = 0; i < data_size; i++)
            {
                device_buffer[i] ^= 0xFF;
            }
            op_count++; // Increment the lifetime ops. counter
            printk(KERN_INFO "accelerator: processed %d bytes (op %d)\n", data_size, op_count);
            break;
        }
        // Wipe the memory clean (fill with zeroes) and reset the state machine
        case ACCEL_RESET:
        {
            memset(device_buffer, 0, BUF_SIZE);
            data_size = 0;
            data_ready = 0;
            printk(KERN_INFO "accelerator: reset\n");
            break;
        }
        case ACCEL_GET_STATUS:
        {
            // The user passed a pointer to an integer. We safely copy 'data_ready' into that integer.
            // arg is cast to an integer pointer sitting in user space.
            if (copy_to_user((int __user *)arg, &data_ready, sizeof(int)))
            {
                return -EFAULT;
            }
            break;
        }
        case ACCEL_GET_COUNT:
        {
           if (copy_to_user((int __user *)arg, &op_count, sizeof(int))) //Same logic, returning the total operations count.
           {
                return -EFAULT;
           }
            break;
        }
        default:
        {
            // If the user sends a command we don't recognize, return Invalid Argument.
            return -EINVAL;
        }
    }
    return 0;
}

/* ===VFS MAPPING=== */
/* Here we map the kernel's standard file ops. to our specific functions. */
static struct file_operations accel_fops = {
    .owner = THIS_MODULE,   // Prevents module from being unloaded while files are open
    .open = accel_open,
    .release = accel_release,
    .read = accel_read,
    .write = accel_write,
    .unlocked_ioctl = accel_ioctl,
};

/* ===MODULE INITIALIZATION=== */
static int __init accel_init(void)
{
    // STEP 1: Allocate memory on the kernel heap.
    // GFP_KERNEL tells the allocator: "You can put the process to sleep if RAM is currently tight."
    device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!device_buffer)
    {
        return -ENOMEM; // Out of memory error
    }
    // STEP 2: Dynamically request a Major device number from the kernel.
    // alloc_chrdev_region(&variable_to_store_it, starting_minor_num, total_devices, name)
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0)
    {
        goto err_free_buf;  // If this fails, jump down and free the memory we just allocated
    }
    // STEP 3: Initialize the cdev structure and attach our file operations map.
    cdev_init(&accel_cdev, &accel_fops);
    // STEP 4: Tell the kernel this device is officially live and ready to receive calls.
    if (cdev_add(&accel_cdev, dev_num, 1) < 0)
    {
        goto err_unreg;
    }
    // STEP 5: Create a "class" in sysfs (/sys/class/accel).
    // This is required so Linux systems (like 'udev') know to automatically 
    // create the physical /dev/accelerator file for us.
    accel_class = class_create(CLASS_NAME);
    if (IS_ERR(accel_class))
    {
        goto err_cdev;
    }
    // STEP 6: Bind the device to the class, officially triggering the creation of the /dev/ node.
    if (IS_ERR(device_create(accel_class, NULL, dev_num, NULL, DEVICE_NAME)))
    {
        goto err_class;
    }
    // Log success. MAJOR(dev_num) extracts just the major number from the dev_t variable.
    printk(KERN_INFO "accelerator: loaded as /dev/%s major=%d\n", DEVICE_NAME, MAJOR(dev_num));
    return 0;

    /* ===GOTO LADDER (Error Handling)=== */
    /* If any step above fails, it jumps here to undo only the steps that succeeded, in reverse order. */
    err_class:
        class_destroy(accel_class);
    err_cdev:
        cdev_del(&accel_cdev);
    err_unreg:
        unregister_chrdev_region(dev_num, 1);
    err_free_buf:
        kfree(device_buffer);
        return -1;
}

/* ===MODULE CELANUP=== */
static void __exit accel_exit(void) 
{
    // We must wind down the module in the EXACT REVERSE order we built it.
    device_destroy(accel_class, dev_num);   // Remove /dev/accelerator
    class_destroy(accel_class);             // Remove /sys/class/accel
    cdev_del(&accel_cdev);                  // Unregister the character device
    unregister_chrdev_region(dev_num, 1);   // Give the major number back to kernel
    kfree(device_buffer);                   // Free the kernel RAM (MUST)
    printk(KERN_INFO "accelerator: unloaded\n");
}

module_init(accel_init);
module_exit(accel_exit);