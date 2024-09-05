#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOC_RESET _IO(SCULL_IOC_MAGIC, 1)

int main()
{
    int fd;
    char buffer[1000];
    char *buf_to_set = "pari pari gusha gusha paki paki. gokun!";

    fd = open("/dev/scull", O_RDWR);
    if (fd < 0) {
        perror("rip");
        return 1;
    }
    if (ioctl(fd, SCULL_IOC_RESET) < 0) {
        perror("rest no work");
        close(fd);
        return 1;
    }
    printf("Buffer reset successful");

    close(fd);
    return 0;
}
