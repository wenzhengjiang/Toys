#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "rlib.h"

#define ACK_PAKET_SIZE 8
#define PACKET_HEADER_SIZE 12
#define EOF_PACKET_SIZE 12

#define bat(r,b,i) b[i%r->window]

enum sender_state {WAITING_DATA, WAITING_ACK};
enum receiver_state {WAITING_PKT, WAITING_FLUSH};

typedef struct sender_pkt {
    packet_t pkt;
    uint32_t len;
    struct timeval send_time;
} sender_pkt_t;

typedef struct receiver_pkt {
    uint8_t data[MAX_PAYLOAD_SIZE];
    uint16_t flushed_bytes;
    uint16_t len;
    bool valid;
    struct receiver_pkt *next;
} receiver_pkt_t;

struct reliable_state {
    rel_t *next;			/* Linked list for traversing all connections */
    rel_t **prev;

    conn_t *c;			/* This is the connection object */

    int timeout;
    int window;

    sender_pkt_t* sbuf; /*packets in sender windows*/
    uint32_t send_ackno, send_seqno; /*sender window*/

    receiver_pkt_t* rbuf; /*packets in receiver window*/
    uint32_t recv_ackno ; /*receiver window*/

    receiver_pkt_t* output_head, *output_tail ;/*Queue List of packets waiting to be flushed */
};

static void add_output(rel_t *r, receiver_pkt_t* recv_pkt);
static void send_ackpkt(conn_t* c, uint32_t ackno);
static bool spkt_timeout(sender_pkt_t *r);
static void pkt_hton(packet_t *p);
static void pkt_ntoh(packet_t *p);
static packet_t* pack_local_data(rel_t *s);
static void before_sendpkt(packet_t *p, size_t pkt_len);
static void after_sendpkt(rel_t *s, packet_t *p, uint32_t pkt_len);
static bool is_bad_pkt(packet_t *p, size_t len);
static void handle_ackpkt(rel_t *s, const packet_t *p);
static void handle_datapkt(rel_t *s, const packet_t *p);
static uint16_t pkt_cksum(packet_t *p, size_t len);
static int tv2millisec(struct timeval t) ;

rel_t *rel_list;

static bool my_opt_debug = true;
/* Creates a new reliable protocol session, returns NULL on failure.
 * Exactly one of c and ss should be NULL.  (ss is NULL when called
 * from rlib.c, while c is NULL when this function is called from
 * rel_demux.) */
    rel_t *
rel_create (conn_t *c, const struct sockaddr_storage *ss,
        const struct config_common *cc)
{
    rel_t *r;

    r = xmalloc (sizeof (*r));
    memset (r, 0, sizeof (*r));

    if (!c) {
        c = conn_create (r, ss);
        if (!c) {
            free (r);
            return NULL;
        }
    }

    r->c = c;
    r->next = rel_list;
    r->prev = &rel_list;
    if (rel_list)
        rel_list->prev = &r->next;
    rel_list = r;

    r->timeout = cc->timeout;
    r->window = cc->window;

    r->sbuf = xmalloc(r->window*sizeof(sender_pkt_t));
    r->send_ackno = 1;
    r->send_seqno = 0; /*window in sender*/

    r->rbuf = xmalloc(r->window*sizeof(receiver_pkt_t));
    r->recv_ackno = 1;
    r->output_head = r->output_tail = NULL;

    return r;
}

    void
rel_destroy (rel_t *r)
{
    if (r->next)
        r->next->prev = r->prev;
    *r->prev = r->next;
    conn_destroy (r->c);

    free(r->sbuf);
    free(r->rbuf);
    while (r->output_head) {
         receiver_pkt_t *p = r->output_head;
         r->output_head = r->output_head->next;
         free(p);
    }
    free(r);
}


/* This function only gets called when the process is running as a
 * server and must handle connections from multiple clients.  You have
 * to look up the rel_t structure based on the address in the
 * sockaddr_storage passed in.  If this is a new connection (sequence
 * number 1), you will need to allocate a new conn_t using rel_create
 * ().  (Pass rel_create NULL for the conn_t, so it will know to
 * allocate a new connection.)
 */
    void
rel_demux (const struct config_common *cc,
        const struct sockaddr_storage *ss,
        packet_t *pkt, size_t len)
{
}

/*Receive a data packet or ack packet*/
    void
rel_recvpkt (rel_t *r, packet_t *pkt, size_t n)
{
    if (is_bad_pkt(pkt, n)) {
        return ;
    }
    pkt_ntoh(pkt);
    if (n == ACK_PAKET_SIZE) {
        handle_ackpkt(r, pkt);
    } else if (n > ACK_PAKET_SIZE){
        handle_datapkt(r, pkt);
    } else {
        fprintf(stderr, "Received packet was too small %d\n", (int)n);
    }
}

/* Create packet from conn_read(); package it; send it */
    void
rel_read (rel_t *s)
{
    if (my_opt_debug) fprintf(stderr, "rel_read called");
    assert(s != NULL);
    if (s->send_seqno-s->send_ackno+1 < s->window) {
        packet_t *pkt = pack_local_data(s);
        // Get a data packet
        if (pkt) {
            int pkt_len = pkt->len;
            before_sendpkt(pkt, pkt_len);
            conn_sendpkt(s->c, pkt, pkt_len);
            after_sendpkt(s, pkt, pkt_len);
            free(pkt);
        }
    }
}

/* Flush buffer. If all buffer is flushed */
    void
rel_output (rel_t *r)
{
    if (opt_debug) fprintf(stderr, "rel_output called\n");
    receiver_pkt_t *h = r->output_head;
    size_t bufspace = conn_bufspace(r->c);
    if (bufspace == 0 || h == NULL) return ;
    size_t rem_bytes = h->len - h->flushed_bytes;
    size_t flush_bytes = bufspace > rem_bytes? rem_bytes : bufspace;

    conn_output(r->c,
            h->data+h->flushed_bytes,
            flush_bytes);
    h->flushed_bytes += flush_bytes;
    if (opt_debug)
        fprintf(stderr, "flushed %d bytes\n", (int)flush_bytes);
    if (h->flushed_bytes == h->len) {
        r->output_head = r->output_head->next;
        free(h);
        rel_output(r);
    }
}

/* Retransmit any packets in sender window that is timeout */
    void
rel_timer ()
{
    for (rel_t *r = rel_list; r; r = r->next) {
        if (r->send_ackno <= r->send_seqno) { /*There are packets not acked yet*/
            for (uint32_t no = r->send_ackno; no <= r->send_seqno; no++) {
                sender_pkt_t *sp = &r->sbuf[no%r->window];
                if (spkt_timeout(sp)) {
                    conn_sendpkt(r->c, &(sp->pkt), sp->len);
                    gettimeofday(&(sp->send_time), NULL);
                }
            }
        }
    }
}

    static void
send_ackpkt(conn_t *c, uint32_t ackno) {
    struct ack_packet ack_pkt;
    ack_pkt.ackno = ackno;
    ack_pkt.len = ACK_PAKET_SIZE;

    before_sendpkt((packet_t*)&ack_pkt, ack_pkt.len);
    conn_sendpkt(c, (packet_t*)&ack_pkt, ACK_PAKET_SIZE);
}

    static int
tv2millisec(struct timeval t) {
    return (int)t.tv_sec*1000 + (int)t.tv_usec/1000;
}
/*Convert a packet to network byte order*/
    static bool
spkt_timeout(sender_pkt_t *sp)
{
    struct timeval now ;
    gettimeofday(&now, NULL);
    return tv2millisec(now) - tv2millisec(sp->send_time);
}

    static void
pkt_hton(packet_t *p)
{
    if (p->len > ACK_PAKET_SIZE)
        p->seqno = htonl(p->seqno);
    p->len = htons(p->len);
    p->ackno = htonl(p->ackno);
}
/*Convert a packet to host byte order*/
    static void
pkt_ntoh(packet_t *p)
{
    p->len = ntohs(p->len);
    p->ackno = ntohl(p->ackno);
    if (p->len > ACK_PAKET_SIZE)
        p->seqno = ntohl(p->seqno);
}

    static packet_t*
pack_local_data(rel_t *s)
{
    packet_t * pkt = xmalloc(sizeof(packet_t));
    int sz = conn_input(s->c, pkt->data, MAX_PAYLOAD_SIZE);
    if (sz== 0) { // No input data
        free(pkt);
        return NULL;
    }
    pkt->len = sz == -1 ? EOF_PACKET_SIZE : PACKET_HEADER_SIZE + sz;
    pkt->ackno = 0; // not used to ack any packet.
    pkt->seqno = s->send_seqno + 1;
    if (my_opt_debug)
        fprintf(stderr, "new data packet %08x %04x\n", pkt->seqno, pkt->len);
    return pkt;
}
/* 1. Change packet bytes order
 * 2. Compute cksum*/

    static void
before_sendpkt(packet_t *p, size_t pkt_len)
{
    pkt_hton(p);
    p->cksum = pkt_cksum(p, pkt_len);
}
/* 1. Backup sent packet in case it needs to be resent
 * 2. Record sending time
 * */
    static void
after_sendpkt(rel_t *s, packet_t *p, uint32_t pkt_len)
{
    struct sender_pkt spkt;
    spkt.pkt = *p;
    spkt.len = pkt_len;
    gettimeofday(&(spkt.send_time), NULL);
    s->sbuf[++(s->send_seqno)%s->window] = spkt;
}

/*Check
 * 1. Whether received all bytes
 * 2. Whether cksum is correct
 * */
    static bool
is_bad_pkt(packet_t *p, size_t rev_len)
{
    int pkt_len = ntohs(p->len);
    if (rev_len < pkt_len) {
        if (opt_debug)
            fprintf(stderr, "Bad packet: too short (%03x, %03x)\n", (int)rev_len, (int)pkt_len);
        return true;
    }
    uint16_t recv_cksum = p->cksum; // network byte order
    uint16_t computed_cksum = pkt_cksum(p, pkt_len);
    if (recv_cksum != computed_cksum) {
        if (opt_debug)
            fprintf(stderr, "Bad packet: incorrect cksum(%04x, %04x)\n", recv_cksum, computed_cksum);
        return true;
    }
    return false;
}
/* Adjust sender window*/

    static void
handle_ackpkt(rel_t *s, const packet_t *p)
{
    if (my_opt_debug) fprintf(stderr, "handling ackpkt %d\n", p->ackno);
    if (p->ackno > s->send_ackno && p->ackno <= s->send_seqno+1) {
        /*change window*/
        s->send_ackno = p->ackno;
    }
}

/*Change receiver window and put data packet in output_buf*/
    void
handle_datapkt(rel_t *s, const packet_t *p)
{
    if (my_opt_debug) fprintf(stderr, "handling datapkt %d\n", p->seqno);
    if (p->seqno >= s->recv_ackno && p->seqno-s->recv_ackno+1 <= s->window) {

        receiver_pkt_t *rpkt = &s->rbuf[p->seqno%s->window];
        rpkt->valid = true;
        rpkt->len = p->len - PACKET_HEADER_SIZE;
        rpkt->flushed_bytes = 0;
        rpkt->next = NULL;
        memcpy(rpkt->data,p->data,rpkt->len);

        bool change = false;
        while (s->rbuf[s->recv_ackno%s->window].valid) {
            add_output(s, &s->rbuf[s->recv_ackno%s->window]);
            s->rbuf[s->recv_ackno%s->window].valid = false;
            s->recv_ackno++;
            change = true;
        }
        if (change) {
            send_ackpkt(s->c, s->recv_ackno);
            rel_output(s);
        }
    } else if (p->seqno < s->recv_ackno) {
         send_ackpkt(s->c, s->recv_ackno);
    }
}

    static void
add_output(rel_t *r, receiver_pkt_t* recv_pkt) {
    receiver_pkt_t *pkt = xmalloc(sizeof(receiver_pkt_t));
    *pkt = *recv_pkt;
    if (r->output_head == NULL) {
         r->output_head = r->output_tail = pkt;
         return ;
    }
    assert(r->output_tail != NULL);
    r->output_tail->next = pkt;
    r->output_tail = pkt;
}
    static uint16_t
pkt_cksum(packet_t *p, size_t len)
{
    uint16_t bk = p->cksum;
    p->cksum = 0;
    uint16_t res = cksum(p, len);
    p->cksum = bk;
    return res;
}
