#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>


int
main(int argc, char *argv[])
{
    int fd;
    extern int errno;

    openlog(argv[0], LOG_PERROR, LOG_USER);

    if (argc < 3) {
        syslog(LOG_DEBUG, "Usage: %s filename string\n", argv[0]);
        exit(1);
    }
    if ((fd = open(argv[1], O_CREAT|O_WRONLY, 0644)) < 0)  {
        syslog(LOG_DEBUG, "unable to open %s: %s", argv[1], strerror(errno));
        exit(1);
    }
    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
    if (write(fd, argv[2], strlen(argv[2])) < 0)  {
        syslog(LOG_DEBUG, "write");
        close(fd);
        exit(1);
    }
    (void)close(fd);
    closelog();
    exit(0);
}