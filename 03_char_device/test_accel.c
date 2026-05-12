#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "accel_ioctl.h"

#define DEVICE "/dev/v_accelerator"


int main()
{
    int fd;
    int status;
    int count;
    char write_buf[] = "Hello from userspace!";
    char read_buf[256];


    printf("=== Virtual Accelerator Test ===\n\n");

    fd = open(DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        return 1;
    }
    printf("Device opened: %s\n", DEVICE);

    printf("Writing: \"%s\"\n", write_buf);
    write(fd, write_buf, strlen(write_buf));

    ioctl(fd, ACCEL_GET_STATUS, &status);
    printf("Status after write: %s\n", status ? "data ready":"no data");

    printf("Processing data (XOR 0xFF transform)....\n");
    ioctl(fd, ACCEL_PROCESS);

    memset(read_buf, 0, sizeof(read_buf));
    int bytes = read(fd, read_buf, sizeof(read_buf));
    printf("Read %d bytes after processing\n",  bytes);

    ioctl(fd, ACCEL_GET_COUNT, &count);
    printf("Total operations: %d\n", count);

    printf("\nProcessing again to reverse XOR.....\n");
    write(fd, read_buf, bytes);
    ioctl(fd, ACCEL_PROCESS);

    memset(read_buf, 0, sizeof(read_buf));
    read(fd, read_buf, sizeof(read_buf));
    printf("After reverse: \"%s\"\n", read_buf);

    printf("\nResetting device...\n");
    ioctl(fd, ACCEL_RESET);

    ioctl(fd, ACCEL_GET_STATUS, &status);
    printf("Status after reset: %s\n",
           status ? "data ready" : "no data");

    close(fd);
    printf("\nDevice closed\n");
    return 0;
}