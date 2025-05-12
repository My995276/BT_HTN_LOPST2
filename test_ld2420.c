#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h> // Include errno for error handling

#define DEVICE_PATH "/dev/hlk_ld2420" // Path to the HLK-LD2420 device

// IOCTL commands for HLK-LD2420
#define HLK_LD2420_IOCTL_MAGIC 'h'
#define HLK_LD2420_IOCTL_READ_DATA _IOR(HLK_LD2420_IOCTL_MAGIC, 1, int)
#define HLK_LD2420_IOCTL_SET_CONFIG _IOW(HLK_LD2420_IOCTL_MAGIC, 2, int)

int main() {
    int fd;
    int data;
    int config = 10; // Example configuration value

    // Open the device
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }

    // Set radar configuration
    if (ioctl(fd, HLK_LD2420_IOCTL_SET_CONFIG, &config) < 0) {
        perror("Failed to set radar configuration");
        close(fd);
        return errno;
    }
    printf("Radar configuration set to: %d\n", config);

    // Read radar data
    if (ioctl(fd, HLK_LD2420_IOCTL_READ_DATA, &data) < 0) {
        perror("Failed to read radar data");
        close(fd);
        return errno;
    }
    printf("Radar data: %d\n", data);

    // Close the device
    close(fd);
    return 0;
}
