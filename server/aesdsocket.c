#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>


#define SERVPORT 9000
#define TRUE 1
#define TMPFILE "/var/tmp/aesdsocketdata"
#define _XOPEN_SOURCE 700

/*
 * globals, at my age?
 */

 FILE *tmp_fp;
 int sock = 0;
 int pid;


void
send_sockdata(FILE *fp)
{
    FILE *sd;
    char buf[BUFSIZ*8];


    if ((sd = fopen(TMPFILE, "r")) == NULL) {
        perror("fopen wtf");
        return;
    } 
    while (fgets(buf, BUFSIZ*8, sd) != 0)  {
        fprintf(fp, "%s", buf);
    }
    fflush(fp);
    fclose(sd);
    return;
}


void
childhandler(int sig)
{
    fclose(tmp_fp);
    (void)unlink(TMPFILE);
    syslog(LOG_DEBUG, "Caught signal, exiting");
    closelog();
    printf("adios\n");
    exit(0);
}


void
parenthandler(int sig)
{
    (void)kill(pid, sig);
}


void
do_service(int sock)
{
    int l_sock;

    socklen_t l_socklen;
    struct sockaddr_in client;
    char namebuf[128];
    FILE *l_file;
    char buf[BUFSIZ*8];

    if (listen(sock, 3) < 0) {
        perror("listen");
        exit(-1);
    }
    if (signal(SIGTERM, childhandler) == SIG_ERR) {
        perror("SIGTERM");
        exit(-1);
    }
    if (signal(SIGINT, childhandler) == SIG_ERR)  {
        perror("SIGERR");
        exit(-1);
    }
    bzero((char *)&client, sizeof client);
    l_socklen = sizeof client;
    if ((tmp_fp = fopen(TMPFILE, "w+")) == NULL)  {
        perror("opening tmp file");
        exit(-1);
    }
    while (TRUE) {
        if ((l_sock = accept(sock, (struct sockaddr *)&client, &l_socklen)) < 0)  {
            perror("accept");
            exit(-1);
        }
        syslog(LOG_DEBUG, "Accepted connection from %s", inet_ntop(AF_INET, (char *)&client.sin_addr, namebuf, 128));
        if ((l_file = fdopen(l_sock, "a+")) == NULL) {
            fprintf(stderr, "fdopen error on socket\n");
            break;
        }
        bzero(buf, BUFSIZ);
        while (fgets(buf, BUFSIZ*8, l_file) != 0)  {
            printf("%s", buf);
            fprintf(tmp_fp, "%s", buf);
            fflush(tmp_fp);
            send_sockdata(l_file);
            bzero(buf, BUFSIZ*8);
        }
    }
}



int
main(int argc, char *argv[])
{
    struct sockaddr_in addr;
    int do_daemon = 0;
    // int pid;
    int status;
    

    if (argc == 2) {
        if (argv[1][0] == '-' && argv[1][1] == 'd') {
            do_daemon = 1;
        } else {
            fprintf(stderr, "Unknown argument\n");
            exit(1);
        }
    }
    openlog(argv[0], LOG_PERROR|LOG_NDELAY|LOG_PID, LOG_USER);
    bzero((char *)&addr, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVPORT);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
        perror("socket");
        exit(-1);
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        exit(-1);
    }
    if (do_daemon == 1) {
        if ((pid = fork()) < 0)  {
            perror("fork");
            exit(-1);
        }
        if (pid == 0)  {
            do_service(sock);
        } else {
            if (signal(SIGTERM, parenthandler) == SIG_ERR) {
                perror("SIGTERM");
                exit(-1);
            }
            if (signal(SIGINT, parenthandler) == SIG_ERR)  {
                perror("SIGERR");
                exit(-1);
            }
            wait(&status);
            exit(0);
        }
    }
    do_service(sock);
}