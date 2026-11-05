/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <SDL_mutex.h>
#include <pcsl_network.h>
#include <pcsl_socket.h>
#include <pcsl_datagram.h>
#include <pcsl_serversocket.h>
extern void ps3me_network_poll(void);
#ifdef PS3ME_NETWORK_TEST
static uint64_t clock_offset;
static int mutex_creations;
SDL_mutex *__real_SDL_CreateMutex(void);
SDL_mutex *__wrap_SDL_CreateMutex(void) {
    ++mutex_creations; return __real_SDL_CreateMutex();
}
static int hold_connect;
uint64_t ps3me_network_test_time_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000+clock_offset;
}
int __real_poll(struct pollfd*,nfds_t,int);
int __wrap_poll(struct pollfd *fds,nfds_t n,int timeout) {
    return hold_connect?0:__real_poll(fds,n,timeout);
}
int __real_connect(int,const struct sockaddr*,socklen_t);
int __wrap_connect(int fd,const struct sockaddr *a,socklen_t n) {
    int r=__real_connect(fd,a,n);
    if(hold_connect) { errno=EINPROGRESS; return -1; }
    return r;
}
#endif
static int events[3], last_handle[3], last_status[3], reject;
int ps3me_network_signal(int h,int kind,int status) {
    if(reject) return 0;
    events[kind-1]++; last_handle[kind-1]=h; last_status[kind-1]=status; return 1;
}
/* PCSL malloc configuration expands to these exported functions. */
void *pcsl_mem_malloc(unsigned int n) { return malloc(n); }
static void tick(void) { ps3me_network_poll(); usleep(1000); }
static void wait_for(int kind,int h) {
    int i;
    for(i=0;i<5000 && last_handle[kind-1]!=h;i++) tick();
    assert(i<5000);
}
static void finish_connect(void *h,int r) {
    int i;
    for(i=0;r==PCSL_NET_WOULDBLOCK && i<5000;i++) { tick(); r=pcsl_socket_open_finish(h,NULL); }
    assert(r==0);
}
static void close_socket(void*h) { void*ctx; assert(pcsl_socket_close_start(h,&ctx)==0); }
static void pair(void **server,void **client) {
    void *listener,*ctx; int port,r; unsigned char ip[]={127,0,0,1};
    assert(pcsl_serversocket_open(0,&listener)==0);
    assert(pcsl_network_getlocalport(listener,&port)==0 && port>0);
    r=pcsl_serversocket_accept_start(listener,server,&ctx); assert(r==PCSL_NET_WOULDBLOCK);
    r=pcsl_socket_open_start(ip,port,client,&ctx); assert(r==0 || r==PCSL_NET_WOULDBLOCK);
    finish_connect(*client,r);
    wait_for(1,(int)(intptr_t)listener);
    assert(pcsl_serversocket_accept_finish(listener,server,&ctx)==0);
    close_socket(listener);
}
int main(int argc,char**argv) {
    void *a,*b,*ctx,*dns; int n,port,r,i,v; unsigned char ip[4],buf[65536]; char text[256];
    assert(pcsl_network_init()==0);
    assert(pcsl_network_gethostbyname_start("127.0.0.1",ip,4,&n,&dns,&ctx)==0 && n==4 && ip[0]==127);
#ifdef PS3ME_NETWORK_TEST
    assert(mutex_creations==0);
    puts("PASS network initialization and numeric IP resolution allocate no DNS mutex");
#endif
    assert(pcsl_network_gethostbyname_start("localhost",ip,4,&n,&dns,&ctx)==PCSL_NET_WOULDBLOCK);
    wait_for(3,(int)(intptr_t)dns);
    assert(pcsl_network_gethostbyname_finish(ip,4,&n,dns,ctx)==0 && n==4 && ip[0]==127);
    puts("PASS asynchronous DNS and numeric IPv4");
    pair(&a,&b);
    assert(pcsl_socket_read_start(a,buf,1,&n,&ctx)==PCSL_NET_WOULDBLOCK);
    i=events[0]; for(n=0;n<25;n++) tick(); assert(events[0]==i);
    assert(pcsl_socket_write_start(b,"hello",5,&n,&ctx)==0 && n==5);
    reject=1; for(n=0;n<10;n++) tick(); assert(events[0]==i); reject=0;
    wait_for(1,(int)(intptr_t)a);
    assert(pcsl_socket_available(a,&n)==0 && n==5);
    assert(pcsl_socket_read_finish(a,buf,5,&n,NULL)==0 && n==5 && !memcmp(buf,"hello",5));
    assert(pcsl_socket_getremoteaddr(a,text)==0 && !strcmp(text,"127.0.0.1"));
    assert(pcsl_network_getremoteport(a,&port)==0 && port>0);
    assert(pcsl_network_setsockopt(a,0,1)==0);
    assert(pcsl_network_getsockopt(a,0,&v)==0 && v==1);
    puts("PASS delayed TCP read, event queue retry, available, addresses and options");
    /* Force partial writes and backpressure; no VM thread may block. */
    assert(pcsl_network_setsockopt(b,4,4096)==0);
    memset(buf,42,sizeof(buf));
    for(i=0;i<4096;i++) { r=pcsl_socket_write_start(b,(char*)buf,sizeof(buf),&n,&ctx); if(r==PCSL_NET_WOULDBLOCK) break; assert(r==0 && n>0); }
    assert(i<4096);
    for(i=0;i<1000;i++) {
        r=pcsl_socket_read_start(a,buf,sizeof(buf),&n,&ctx);
        assert(r==0 || r==PCSL_NET_WOULDBLOCK); tick();
        r=pcsl_socket_write_finish(b,"x",1,&n,NULL);
        if(r==0) break; assert(r==PCSL_NET_WOULDBLOCK);
    }
    assert(i<1000); puts("PASS partial writes and send backpressure");
    close_socket(a); close_socket(b);
    pair(&a,&b);
    assert(pcsl_socket_shutdown_output(b)==0);
    for(i=0;i<1000;i++) { r=pcsl_socket_read_start(a,buf,1,&n,&ctx); if(!r) break; tick(); }
    assert(r==0 && n==0); puts("PASS TCP EOF and half-close");
    close_socket(a); close_socket(b);
    pair(&a,&b);
    assert(pcsl_socket_read_start(a,buf,1,&n,&ctx)==PCSL_NET_WOULDBLOCK);
    reject=1; close_socket(a); reject=0;
    wait_for(1,(int)(intptr_t)a); assert(last_status[0]==PCSL_NET_INTERRUPTED);
    assert(pcsl_socket_read_finish(a,buf,1,&n,NULL)==PCSL_NET_INTERRUPTED);
    close_socket(b); puts("PASS closing blocked read, stale handles and full event queue");
    assert(pcsl_datagram_open_start(0,&a,&ctx)==0); assert(pcsl_datagram_open_start(0,&b,&ctx)==0);
    assert(pcsl_network_getlocalport(a,&port)==0);
    assert(pcsl_datagram_read_start(a,ip,&v,(char*)buf,10,&n,&ctx)==PCSL_NET_WOULDBLOCK);
    { unsigned char loop[]={127,0,0,1}; assert(pcsl_datagram_write_start(b,loop,port,"udp",3,&n,&ctx)==0 && n==3); }
    wait_for(1,(int)(intptr_t)a);
    assert(pcsl_datagram_read_finish(a,ip,&v,(char*)buf,10,&n,NULL)==0 && n==3 && !memcmp(buf,"udp",3) && ip[0]==127 && v>0);
    close_socket(a); close_socket(b); puts("PASS UDP send/receive and source endpoint");
    /* Repeated sessions must neither leak descriptors nor recycle MIDP IDs. */
    for(i=0;i<100;i++) { pair(&a,&b); assert(pcsl_network_finalize_start(NULL)==0); assert(pcsl_socket_read_finish(a,buf,1,&n,NULL)==PCSL_NET_INTERRUPTED); }
    puts("PASS 100 sessions and teardown");
#ifdef PS3ME_NETWORK_TEST
    /* A blackholed connect must wake its Java waiter, even with a full queue.
     * Only OS readiness and elapsed time are controlled; the real socket and
     * production PCSL start/poll/finish lifecycle are exercised. */
    {
        void *listener; unsigned char loop[]={127,0,0,1}; int before;
        assert(pcsl_serversocket_open(0,&listener)==0);
        assert(pcsl_network_getlocalport(listener,&port)==0);
        hold_connect=1;
        assert(pcsl_socket_open_start(loop,port,&a,&ctx)==PCSL_NET_WOULDBLOCK);
        assert(pcsl_socket_open_finish(a,NULL)==PCSL_NET_WOULDBLOCK);
        before=events[1]; reject=1; clock_offset+=15001;
        tick(); assert(events[1]==before); reject=0;
        wait_for(2,(int)(intptr_t)a);
        assert(last_status[1]==PCSL_NET_IOERROR);
        assert(pcsl_socket_open_finish(a,NULL)==PCSL_NET_IOERROR);
        assert(pcsl_network_error(a)==ETIMEDOUT);
        hold_connect=0; close_socket(listener);
        /* Opening a new connection still succeeds after the failed one. */
        pair(&a,&b); close_socket(a); close_socket(b);
        puts("PASS stalled-connect timeout, queued wakeup and recovery");
    }
#endif
    /* Optional read-only check of the exact Discord 5.3.1 default endpoints. */
    if(argc>1 && !strcmp(argv[1],"--discord")) {
        unsigned char proxy[]={146,59,80,3}; int total=0;
        r=pcsl_socket_open_start(proxy,80,&a,&ctx); finish_connect(a,r);
        { char req[]="GET /api/v9/users/@me HTTP/1.1\r\nHost: 146.59.80.3\r\nConnection: close\r\n\r\n";
          assert(pcsl_socket_write_start(a,req,strlen(req),&n,&ctx)==0); }
        for(i=0;i<10000 && total<4095;i++) {
            r=pcsl_socket_read_start(a,buf+total,4095-total,&n,&ctx);
            if(r==PCSL_NET_WOULDBLOCK) { tick(); continue; }
            assert(r==0); if(!n) break; total+=n;
        }
        buf[total]=0; assert(strstr((char*)buf,"HTTP/1.1") || strstr((char*)buf,"HTTP/1.0"));
        printf("Discord unauthenticated HTTP response: %.80s\n",buf); close_socket(a);
        r=pcsl_socket_open_start(proxy,8081,&a,&ctx); finish_connect(a,r); close_socket(a);
        puts("PASS Discord default HTTP and gateway TCP reachability (no account login)");
    }
    return 0;
}
