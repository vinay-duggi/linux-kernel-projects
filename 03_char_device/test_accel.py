#!/usr/bin/env python3
import fcntl
import struct
import os

DEVICE = "/dev/v_accelerator"

ACCEL_MAGIC      = ord('A')
ACCEL_PROCESS    = (ACCEL_MAGIC << 8) | 1
ACCEL_RESET      = (ACCEL_MAGIC << 8) | 2
ACCEL_GET_STATUS = (0x80000000 | (4 << 16) | (ACCEL_MAGIC << 8) | 3)
ACCEL_GET_COUNT  = (0x80000000 | (4 << 16) | (ACCEL_MAGIC << 8) | 4)

def main():
    print("=== Virtual Accelerator Python Test ===\n")

    fd = os.open(DEVICE, os.O_RDWR)
    print(f"Opened {DEVICE}")

    data = b"Hello from Python!"
    os.write(fd, data)
    print(f"Written: {data}")

    buf = struct.pack('i', 0)
    result = fcntl.ioctl(fd, ACCEL_GET_STATUS, buf)
    status = struct.unpack('i', result)[0]
    print(f"Status: {'ready' if status else 'empty'}")

    fcntl.ioctl(fd, ACCEL_PROCESS)
    print("Processed data")

    result = os.read(fd, 256)
    print(f"Read back: {result}")

    buf = struct.pack('i', 0)
    result = fcntl.ioctl(fd, ACCEL_GET_COUNT, buf)
    count = struct.unpack('i', result)[0]
    print(f"Total operations: {count}")

    os.close(fd)
    print("Device closed")

if __name__ == '__main__':
    main()
