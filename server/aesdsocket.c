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
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/queue.h>

#define SERVPORT 9000
#define TRUE 1
#define TMPFILE "/var/tmp/aesdsocketdata"
#define _XOPEN_SOURCE 700


enum threadstate {
    TS_ALIVE,
    TS_DONE
};

struct tmpf {
    FILE *tmp_fp;
    pthread_mutex_t mutex;
 } tmpf;

struct entry {
    pthread_t tid;
    enum threadstate tstate;
    int sock_fd;
    LIST_ENTRY(entry) entries;
};

LIST_HEAD(listhead, entry);

static int nthreads;
static int nlive;
int sock = 0;
int pid;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;


void
writetmpfile(char *str)
{
    pthread_mutex_lock(&tmpf.mutex);
    fprintf(tmpf.tmp_fp, "%s", str);
    fflush(tmpf.tmp_fp);
    pthread_mutex_unlock(&tmpf.mutex);
}

void
dotimestamp(int sig)
{
    time_t t;
    struct tm *loc_time;
    char timestr[64];

    if (time(&t) < 0)  {
        perror("time");
        exit(-1);
    }
    if ((loc_time = localtime(&t)) == NULL) {
        perror("localtime");
        exit(-1);
    }
    (void)strftime(timestr, 64, "timestamp: %a, %d %b %Y %T %z\n", loc_time);
    writetmpfile(timestr);

}

void
setalarm(int secs)
{
    struct itimerval alarmtime;

    alarmtime.it_interval.tv_sec = secs;
    alarmtime.it_interval.tv_usec = 0;
    alarmtime.it_value.tv_sec = secs;
    alarmtime.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &alarmtime, NULL) < 0) {
        perror("setitimer");
        exit(-1);
    }
}

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
handler(int sig)
{
    fclose(tmpf.tmp_fp);
    (void)unlink(TMPFILE);
    syslog(LOG_DEBUG, "Caught signal, exiting");
    closelog();
    exit(0);
}


void *
worker(void *arg)
{
    struct entry *td = (struct entry *)arg;
    FILE *l_file;
    char buf[BUFSIZ*8];

    if ((l_file = fdopen(td->sock_fd, "a+")) == NULL) {
        fprintf(stderr, "fdopen error on socket\n");
    } else {
        bzero(buf, BUFSIZ);
        while (fgets(buf, BUFSIZ*8, l_file) != 0)  {
            printf("%s", buf);
            writetmpfile(buf);
            send_sockdata(l_file);
            bzero(buf, BUFSIZ*8);
        }
    }
    while (pthread_mutex_trylock(&mut) != 0)
        ;
    td->tstate = TS_DONE;
    pthread_mutex_unlock(&mut);
}

void
doservice(int sock, int isdaemon)
{
    int pid;
    socklen_t l_socklen;
    struct sockaddr_in client;
    char namebuf[128];
    int l_sock;
    struct entry *threaddata, *np;
    struct listhead head;
    

    if (isdaemon == TRUE) {
        pid = fork();
        if (pid != 0)
            exit(0);
        pid = fork();
        if (pid != 0)
            exit(0);
    }
    setalarm(10);
    if (signal(SIGALRM, dotimestamp) == SIG_ERR)  {
        perror("SIGALRM");
        exit(-1);
    }
    if (listen(sock, 3) < 0) {
        perror("listen");
        exit(-1);
    }
    if (signal(SIGTERM, handler) == SIG_ERR) {
        perror("SIGTERM");
        exit(-1);
    }
    if (signal(SIGINT, handler) == SIG_ERR)  {
        perror("SIGERR");
        exit(-1);
    }
    LIST_INIT(&head);
    while (TRUE) {
        if ((l_sock = accept(sock, (struct sockaddr *)&client, &l_socklen)) < 0)  {
            perror("accept");
            exit(-1);
        }
        syslog(LOG_DEBUG, "Accepted connection from %s", inet_ntop(AF_INET, 
            (char *)&client.sin_addr, namebuf, 128));
        if ((threaddata = (struct entry *)malloc(sizeof(struct entry))) == (struct entry *)NULL)  {
            fprintf(stderr, "malloc failure\n");
            break;
        }
        threaddata->tstate = TS_ALIVE;
        threaddata->sock_fd = l_sock;
        (void)pthread_create(&threaddata->tid, NULL, worker, (void *)threaddata);
        pthread_mutex_trylock(&mut);
        LIST_INSERT_HEAD(&head, threaddata, entries);
        pthread_mutex_unlock(&mut);
        LIST_FOREACH(np, &head, entries) {
            if (np->tstate == TS_DONE)  {
                if (pthread_mutex_trylock(&mut) == 0)  {
                    LIST_REMOVE(np, entries);
                    pthread_mutex_unlock(&mut);
                    if (pthread_join(np->tid, NULL) != 0)
                        perror("pthread_join");
                    free(np);
                }                
            }
        }
        // if ((l_file = fdopen(l_sock, "a+")) == NULL) {
        //     fprintf(stderr, "fdopen error on socket\n");
        //     break;
        // }
        // bzero(buf, BUFSIZ);
        // while (fgets(buf, BUFSIZ*8, l_file) != 0)  {
        //     printf("%s", buf);
        //     writetmpfile(buf);
        //     send_sockdata(l_file);
        //     bzero(buf, BUFSIZ*8);
        // }

        
    }
}


int main(int argc, char *argv[])
{
    int nthreads = 10;
    int livethreads = nthreads;
    int i, j;
    struct sockaddr_in addr;
    int do_daemon = 0;   

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
    if ((tmpf.tmp_fp = fopen(TMPFILE, "w+")) == NULL)  {
        perror("opening tmp file");
        exit(-1);
    }
    doservice(sock, do_daemon);

    // for ( i = 0 ; i < nthreads ; i++ )  {


    // }
    // while (livethreads != 0)  {
    //     LIST_FOREACH(np, &head, entries)  {
    //         printf("thread 0x%lx, tstate = %d\n", np->tid, (int)np->tstate);
    //         if (np->tstate = TS_DONE)  {
    //             pthread_join(np->tid, NULL);
    //             livethreads--;
    //             LIST_REMOVE(np, entries);
    //         }
    //         pthread_mutex_unlock(&mut);
    //     }
    // }

    exit(0);
}