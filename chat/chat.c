#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>

#define ECHO_PORT 9999
#define BUF_SIZE 4096
#define TIME_INF -1
#define CON_BUF_SIZE 8192

struct chunk {
    struct chunk *next;
    size_t size;
    size_t used;
    char buf[1];
};

typedef struct chunk chunk_t;

struct conn {

    int rpoll;			/* offsets into cevents array */
    int wpoll;

    int rfd;			/* input file descriptor */
    int wfd;			/* output file descriptor */

    char * id;

    /* When write_err is true, stop reading and writing and free the conn*/
    bool write_err;
    /* When delete_me is true, stop reading, and free the conn after draining buffer */
    bool delete_me;
    chunk_t *outq; /* chunks not yet written */
    chunk_t **outqtail;

    struct conn *next; /* Linked list of connections */
    struct conn ** prev;
};

typedef struct conn conn_t;

static conn_t *conn_list;
static size_t idssize;

static struct pollfd *cevents;
static int ncevents;
static conn_t **evreaders, **evwriters;

static char buf[BUF_SIZE];

void *
xmalloc (size_t n)
{
    void *p = malloc (n);
    if (!p) {
        fprintf (stderr, "out of memory allocating %d bytes\n",
                 (int) n);
        abort ();
    }
    return p;
}
/*Init events in events list, read conns, write conns */
void
conn_mkevents ()
{
    struct pollfd *e;
    conn_t **r, **w;
    size_t n = 2;
    conn_t *c;

    /* Map conns to events */
    for (c = conn_list; c; c = c->next) {
        if (c->delete_me) {
            c->rpoll = 0;
            if (c->write_err)
                c->wpoll = 0;
            else
                c->wpoll = n++;
        } else {
            c->rpoll = n++;
            if (c->write_err)
                c->wpoll = 0;
            else
                c->wpoll = c->rpoll;
        }
    }
    e = xmalloc (n * sizeof (*e));
    memset(e, 0, n * sizeof (*e));
    if (cevents)
        e[0] = cevents[0];
    else
        e[0].fd = -1;
    e[1].fd = 2;

    for (c = conn_list; c; c = c->next) {
        if (c->rpoll) {
            e[c->rpoll].fd = c->rfd;
            e[c->rpoll].events |= POLLIN;
#if 0
            fprintf(stderr, "Add reader fd=%d,poll=%d\n", c->rfd, c->rpoll);
#endif
        }
        if (c->wpoll) {
            e[c->wpoll].fd = c->wfd;
            e[c->wpoll].events |= POLLOUT;
#if 0
            fprintf(stderr, "Add writer fd=%d,poll=%d\n", c->wfd, c->wpoll);
#endif

        }
    }
    r = xmalloc (n * sizeof (*r));
    memset(r, 0, n * sizeof (*r));
    w = xmalloc (n * sizeof (*w));
    memset(w, 0, n * sizeof (*w));
    for (c = conn_list; c; c = c->next) {
        if (c->rpoll > 0)
            r[c->rpoll] = c;
        if (c->wpoll > 0)
            w[c->wpoll] = c;
    }
    free(cevents);
    cevents = e;
    ncevents = n;
    free (evreaders);
    evreaders = r;
    free (evwriters);
    evwriters = w;
}

conn_t *
conn_alloc (void)
{
    conn_t *c = xmalloc (sizeof (*c));
    memset (c, 0, sizeof (*c));
    c->prev = &conn_list;
    c->next = conn_list;
    c->outqtail = &c->outq;
    if (conn_list)
        conn_list->prev = &c->next;
    conn_list = c;

    return c;
}

void
conn_free (conn_t *c)
{
    chunk_t *ch, *nch;

    for (ch = c->outq; ch; ch = nch) {
        nch = ch->next;
        free (ch);
    }
    if (c->next)
        c->next->prev = c->prev;
    *c->prev = c->next;

    close (c->rfd);
    if (c->wfd != c->rfd)
        close (c->wfd);
    /* to help catch errors */
    memset (c, 0xc5, sizeof (*c));
    free (c);

    /* update event list */
    conn_mkevents();
}
/* Get some input from the reliable side. This
 * function returns the number of bytes received, 0 if there is no
 * data currently available, and -1 on EOF or error. */

int
conn_input(conn_t *c, void *buf, size_t n)
{
    cevents[c->rpoll].events &= ~POLLIN;
    assert(!c->delete_me);
    int r = read(c->rfd, buf, n);
    if (r == 0 || (r < 0 && errno != EAGAIN)) {
        if (r == 0)
            errno = EIO;
        r = -1;
        return r;
    }
    if (r < 0 && errno == EAGAIN)
        r = 0;
    cevents[c->rpoll].events |= POLLIN;
    return r;
}

/* remain space in conn buffer */
size_t
conn_bufspace (conn_t *c)
{
    chunk_t *ch;
    size_t used = 0;
    const size_t bufsize = CON_BUF_SIZE;

    for (ch = c->outq; ch; ch = ch->next)
        used += (ch->size - ch->used);
    return used > bufsize ? 0 : bufsize - used;
}
/* store _buf to conn buffer */
void
conn_store(conn_t *c, const void *_buf, size_t _n)
{
    const char *buf = _buf;
    int n = _n;
#ifdef DEBUG
        fprintf(stderr, "conn_store: %s\n", _buf);
#endif

    assert (!c->delete_me && conn_bufspace(c));
    if (n > 0) {
        chunk_t *ch = xmalloc (offsetof (chunk_t, buf[n]));
        ch->next = NULL;
        ch->size = n;
        ch->used = 0;
        memcpy (ch->buf, buf, n);
        *c->outqtail = ch;
        c->outqtail = &ch->next;
    }
}

/* Drain output buffer */
int
conn_drain (conn_t *c)
{
    chunk_t *ch;

    if (c->wpoll)
        cevents[c->wpoll].events &= ~POLLOUT;

    if (c->write_err)
        return -1;

    while ((ch = c->outq)) {
        int n = write (c->wfd, ch->buf + ch->used,
                ch->size - ch->used);
        if (n < 0) {
            if (errno != EAGAIN) {
                c->write_err = 1;
                return -1;
            }
            break;
        }
        ch->used += n;
        if (ch->used < ch->size) {
            if (c->wpoll)
                cevents[c->wpoll].events |= POLLOUT;
            break;
        }
        c->outq = ch->next;
        if (!c->outq)
            c->outqtail = &c->outq;
        free (ch);
    }
    cevents[c->wpoll].events |= POLLOUT;
    return 0;
}

void send_user_list(conn_t *dest)
{
    char *buf = xmalloc(idssize);
    int k = 0;
    for(conn_t *c = conn_list; c; c = c->next) {
        assert(c->id != NULL);
        strcpy(buf+k, c->id);
        k += strlen(c->id);
    }
#ifdef DDEBUG
    buf[k] = '\0'
    fprintf(stderr, "send_user_liste: %s\n", buf);
#endif
    conn_store(dest, buf, k);
}

conn_t * getconnbyid(char *id) {
    for (conn_t *c = conn_list; c; c = c->next) {
        if (strcmp(id, c->id) == 0)
            return c;
    }
    return NULL;
}

void handle_cmd(conn_t *c, char *buf, int len)
{
    buf[len] = '\0';
#if DEBUG
    fprintf(stderr, "handle_cmd:");
    for (int i = 0; i < len; i++)
    fprintf(stderr, "%c", buf[i]);
#endif
    char *errmsg = xmalloc(BUF_SIZE);

    if (strcmp(buf, "list\r\n") == 0) {
        send_user_list(c);
    } else {
        if (len > 5 && buf[0] == 'c' && buf[1] == 'o'
                && buf[2] == 'n' && buf[3] == 'n' && buf[4] == ' ') {
            conn_t *dest = getconnbyid(&buf[5]);
            if (dest == NULL) {
                sprintf(errmsg, "no such user: %s\n", buf+5);
                conn_store(c, errmsg, strlen(errmsg));
            } else {
                c->wfd = dest->rfd;
            }
        } else {
            sprintf(errmsg, "no such command: %s\n", buf);
            conn_store(c, errmsg, strlen(errmsg));
        }
    }
}

int
make_async(int fd)
{
     int n;
    if ((n = fcntl (fd, F_GETFL)) < 0
            || fcntl (fd, F_SETFL, n | O_NONBLOCK) < 0)
        return -1;
    return 0;
}

/* main event loop */
void conn_poll()
{
    int n = poll(cevents, ncevents, TIME_INF);
    if (n == -1 && errno != EAGAIN) {
        perror("poll");
        exit(1);
    }
    conn_t *c, *nc ;
    for (int i = 1; i < ncevents; i++) {
        if (cevents[i].revents & (POLLIN|POLLERR|POLLHUP)) {
                /* reading now will not block */
            assert(c = evreaders[i]);
            /* if reader connection is open and connection buffer is not full */
            if (!c->delete_me && conn_bufspace(c)) {
                int n = conn_input(c, buf, BUF_SIZE);
                if (n > 0 && !c->delete_me) {
                    if (c->rfd == c->wfd)  /* command from client who hasn't connected to another client */
                        handle_cmd(c, buf, n);
                    else                 /* char message needs to be sent to another client */
                        conn_store(c, buf, n);
                }
                else
                    c->delete_me = true;
#if DEBUG
                fprintf(stderr, "received %d bytes", n);
#endif
            }
        }
        if (cevents[i].revents & (POLLOUT|POLLHUP|POLLERR)) {
                /* writing now will not block */
            assert(c = evwriters[i]);
            if (conn_drain(c) < 0)
                c->delete_me = true;
        }
        if (cevents[i].revents & (POLLHUP|POLLERR)) {
#if DEBUG
            fprintf(stderr, "%5d Error on fd %d (0x%x)\n",
                    getpid(), cevents[i].fd, cevents[i].revents);
#endif
            if (cevents[i].fd == 2)
                exit(1);
            cevents[i].fd = -1;
        }
        cevents[i].revents = 0;
    }
    for (c = conn_list; c ; c = nc) {
         nc = c->next;
         if (c->delete_me && (c->write_err || !c->outq))
             conn_free(c);
    }
}

/* id = host:port */
char *gen_id(int sock)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    if(getpeername(sock, (struct sockaddr*)&addr, &len) < 0) {
        perror("getpeername");
        return NULL;
    }
    char port[8];
    char host[1024];
    char service[20];
    if(getnameinfo((struct sockaddr*)&addr, sizeof addr, host, sizeof host, service, sizeof service, 0) < 0) {
        perror("getnameinfo");
        return NULL;
    }
    sprintf(port, "%d", ntohs(addr.sin_port));
    char *buf = xmalloc(strlen(host)+strlen(host)+1);
    sprintf(buf, "%s:%s", host, port);
    return buf;
}

void
do_server(int listenfd) {
    conn_mkevents ();
    if (make_async(listenfd) < 0) {
        perror("do_server");
        exit(1);
    }
    cevents[0].fd = listenfd;
    cevents[0].events = POLLIN;
    for (;;) {
        conn_poll();
        if (cevents[0].revents) { /* new connection */
            struct sockaddr_storage ss;
            socklen_t len = sizeof(ss);
            int cli = accept(listenfd, (struct sockaddr *)&ss, &len);
            if (cli < 0 && errno != EAGAIN)
                perror("accept");
            if (cli < 0)
                continue;
            fprintf(stderr, "server: new connection\n");
            make_async(cli);
            conn_t *c = conn_alloc();
            c->rfd = c->wfd = cli;
            c->id = gen_id(cli);
            idssize += strlen(c->id);
            conn_mkevents();
        }
    }
}

    int
main(int argc, char* argv[])
{
    int listenfd;
    struct sockaddr_in servaddr;
    struct sigaction sa;

    /* Ignore SIGPIPE, since we may get a lot of these */
    /*SIGPIPE: Try to write to a pipe that has no reader*/
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = SIG_IGN;
    sigaction (SIGPIPE, &sa, NULL);

    fprintf(stdout, "----- Chat Server -----\n");

    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ECHO_PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd , (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
         close(listenfd);
         perror("bind");
         return EXIT_FAILURE;
    }

    if (listen(listenfd, 5) < 0) {
         close(listenfd);
         perror("listen");
         return EXIT_FAILURE;
    }

    do_server(listenfd);

    return EXIT_SUCCESS;
}

