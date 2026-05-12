#ifndef ACCEL_IOCTL_H
#define ACCEL_IOCTL_H

#include <sys/ioctl.h>

#define ACCEL_MAGIC         'A'
#define ACCEL_PROCESS       _IO(ACCEL_MAGIC,  1)
#define ACCEL_RESET         _IO(ACCEL_MAGIC,  2)
#define ACCEL_GET_STATUS    _IOR(ACCEL_MAGIC, 3, int)
#define ACCEL_GET_COUNT     _IOR(ACCEL_MAGIC, 4, int)

#endif
