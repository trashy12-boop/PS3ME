/* SPDX-License-Identifier: GPL-2.0-only
 * PS3ME PCSL transport. VM thread owns sockets; DNS workers never touch the VM.
 * Small monotonically increasing handles fit MIDP's jint descriptors on PPU64.
 * No blocking connect/read/write/accept calls run on the Java scheduler thread.
 */
#include <pcsl_network.h>
#include <pcsl_socket.h>
#include <pcsl_datagram.h>
#include <pcsl_serversocket.h>
#include <pcsl_network_notifier.h>
#include <pcsl_memory.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef PS3ME_PPU
#include <net/net.h>
#include <net/netctl.h>
#include <net/poll.h>
#define NETERR net_errno
#define AGAIN NET_EWOULDBLOCK
#define INPROGRESS NET_EINPROGRESS
#define ALREADY NET_EALREADY
#define INTERRUPTED NET_EINTR
#define CLOSE(fd) netClose((fd)&~SOCKET_FD_MASK)
/* PSL1GHT's SDK has no netinet/tcp.h. */
#define TCP_NODELAY 1
#else
#include <netinet/tcp.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#define NETERR errno
#define AGAIN EWOULDBLOCK
#define INPROGRESS EINPROGRESS
#define ALREADY EALREADY
#define INTERRUPTED EINTR
#define CLOSE close
#endif
#define SOCKETS 32
#define DNS_JOBS 4
#define READ_WAIT 1
#define WRITE_WAIT 2
#define LOOKUP_WAIT 3
/* Returns nonzero only when the event was queued; a full queue is retried. */
extern int ps3me_network_signal(int handle, int kind, int status);
typedef struct { int id, fd, wait; } Socket;
typedef struct {
    int id, done, notified, abandoned, result;
    char name[256]; unsigned char address[4]; SDL_Thread *thread;
} Lookup;
static Socket sockets[SOCKETS];
static Lookup lookups[DNS_JOBS];
static SDL_mutex *dns_lock;
static int initialized, next_id = 1, last_error;
static int new_id(void) {
    /* Never reuse descriptors, including after a MIDlet session ends. */
    return next_id < 0x7fffffff ? next_id++ : 0;
}
static Socket *find(void *h) {
    int i, id = (int)(intptr_t)h;
    for (i=0;i<SOCKETS;i++) if(sockets[i].id && sockets[i].id==id && sockets[i].fd>=0) return &sockets[i];
    return NULL;
}
static void *handle(Socket *s) { return (void*)(intptr_t)s->id; }
static int failed(void) { last_error=NETERR; return PCSL_NET_IOERROR; }
static int wouldblock(void) { return NETERR==AGAIN || NETERR==INPROGRESS || NETERR==ALREADY || NETERR==INTERRUPTED; }
static int nonblock(int fd) {
#ifdef PS3ME_PPU
    int on=1; return setsockopt(fd,SOL_SOCKET,SO_NBIO,&on,sizeof(on));
#else
    int f=fcntl(fd,F_GETFL,0); return f<0 ? -1 : fcntl(fd,F_SETFL,f|O_NONBLOCK);
#endif
}
static void address(struct sockaddr_in *a, const unsigned char *ip, int port) {
    memset(a,0,sizeof(*a)); a->sin_family=AF_INET; a->sin_port=htons((unsigned short)port);
#ifdef PS3ME_PPU
    a->sin_len=sizeof(*a);
#endif
    if(ip) memcpy(&a->sin_addr,ip,4);
}
static Socket *adopt(int fd) {
    int i;
    if(nonblock(fd)<0) { last_error=NETERR; CLOSE(fd); return NULL; }
    for(i=0;i<SOCKETS;i++) if(!sockets[i].id) {
        int id=new_id(); if(!id) break;
        sockets[i].id=id; sockets[i].fd=fd; sockets[i].wait=0; return &sockets[i];
    }
    CLOSE(fd); return NULL;
}
static void discard(Socket *s) { if(s->fd>=0) CLOSE(s->fd); memset(s,0,sizeof(*s)); }
int pcsl_network_init_start(PCSL_NET_CALLBACK cb) {
    (void)cb;
    if(initialized) return PCSL_NET_SUCCESS;
#ifdef PS3ME_PPU
    if(netInitialize()<0) return PCSL_NET_IOERROR;
    /* netctl is used for querying the console's configured interface only. */
    netCtlInit();
#endif
    dns_lock=SDL_CreateMutex();
    if(!dns_lock) return PCSL_NET_IOERROR;
    initialized=1; return PCSL_NET_SUCCESS;
}
int pcsl_network_init(void) { return pcsl_network_init_start(NULL); }
int pcsl_network_init_finish(void) { return PCSL_NET_SUCCESS; }
/* Keep process-wide networking alive: outstanding DNS workers own no VM data.
 * Closing a session abandons their results instead of waiting for DNS timeout. */
int pcsl_network_finalize_start(PCSL_NET_CALLBACK cb) {
    int i; (void)cb;
    for(i=0;i<SOCKETS;i++) if(sockets[i].id) discard(&sockets[i]);
    if(dns_lock) {
        SDL_LockMutex(dns_lock);
        for(i=0;i<DNS_JOBS;i++) if(lookups[i].id) lookups[i].abandoned=1;
        SDL_UnlockMutex(dns_lock);
    }
    return PCSL_NET_SUCCESS;
}
int pcsl_network_finalize_finish(void) { return PCSL_NET_SUCCESS; }
static int resolve(void *arg) {
    Lookup *job=(Lookup*)arg; unsigned char bytes[4]; int status=PCSL_NET_IOERROR;
    /* gethostbyname may use static storage; only one resolver runs at once. */
    SDL_mutex *resolver_lock;
    /* Created by the scheduler before starting any workers (see start below). */
    extern SDL_mutex *ps3me_dns_resolver_lock;
    resolver_lock=ps3me_dns_resolver_lock;
    SDL_LockMutex(resolver_lock);
    {
        struct hostent *h=gethostbyname(job->name);
        if(h && h->h_addrtype==AF_INET && h->h_length==4 && h->h_addr_list[0]) {
            memcpy(bytes,h->h_addr_list[0],4); status=PCSL_NET_SUCCESS;
        }
    }
    SDL_UnlockMutex(resolver_lock);
    SDL_LockMutex(dns_lock);
    if(status==PCSL_NET_SUCCESS) memcpy(job->address,bytes,4);
    job->result=status; job->done=1;
    SDL_UnlockMutex(dns_lock);
    return 0;
}
SDL_mutex *ps3me_dns_resolver_lock;
static void reap(Lookup *j) {
    /* Only call after done; worker has released dns_lock before being joined. */
    SDL_Thread *t=j->thread;
    SDL_UnlockMutex(dns_lock); SDL_WaitThread(t,NULL); SDL_LockMutex(dns_lock);
    memset(j,0,sizeof(*j));
}
int pcsl_network_gethostbyname_start(char *name,unsigned char *out,int max,int *len,void **h,void **ctx) {
    int i; struct in_addr ip;
    if(!name || !out || max<4 || !len || !h || !ctx || strlen(name)>=256) return PCSL_NET_INVALID;
    *h=NULL; *ctx=NULL;
    if(pcsl_network_init()!=0) return PCSL_NET_IOERROR;
    if(inet_pton(AF_INET,name,&ip)==1) { memcpy(out,&ip,4); *len=4; return PCSL_NET_SUCCESS; }
    if(!ps3me_dns_resolver_lock) ps3me_dns_resolver_lock=SDL_CreateMutex();
    if(!ps3me_dns_resolver_lock) return PCSL_NET_IOERROR;
    SDL_LockMutex(dns_lock);
    for(i=0;i<DNS_JOBS;i++) {
        Lookup *j=&lookups[i];
        if(j->id && j->done && j->abandoned) reap(j);
        if(!j->id) {
            j->id=new_id(); if(!j->id) break;
            strcpy(j->name,name);
            j->thread=SDL_CreateThread(resolve,j);
            if(!j->thread) { memset(j,0,sizeof(*j)); break; }
            *h=(void*)(intptr_t)j->id;
            SDL_UnlockMutex(dns_lock); return PCSL_NET_WOULDBLOCK;
        }
    }
    SDL_UnlockMutex(dns_lock); return PCSL_NET_IOERROR;
}
int pcsl_network_gethostbyname_finish(unsigned char *out,int max,int *len,void *h,void *ctx) {
    int i, result=PCSL_NET_IOERROR; (void)ctx;
    if(!dns_lock || !out || max<4 || !len) return PCSL_NET_INVALID;
    SDL_LockMutex(dns_lock);
    for(i=0;i<DNS_JOBS;i++) if(lookups[i].id==(int)(intptr_t)h) {
        Lookup *j=&lookups[i];
        if(!j->done) { result=PCSL_NET_WOULDBLOCK; break; }
        result=j->result;
        if(result==0) { memcpy(out,j->address,4); *len=4; }
        reap(j); break;
    }
    SDL_UnlockMutex(dns_lock); return result;
}
void ps3me_network_poll(void) {
    struct pollfd fds[SOCKETS]; Socket *map[SOCKETS]; int i,n=0;
    if(!initialized) return;
    for(i=0;i<SOCKETS;i++) if(sockets[i].id && sockets[i].wait) {
        Socket *s=&sockets[i];
        if(s->fd<0) {
            if((s->wait&READ_WAIT) && ps3me_network_signal(s->id,READ_WAIT,PCSL_NET_INTERRUPTED)) s->wait&=~READ_WAIT;
            if((s->wait&WRITE_WAIT) && ps3me_network_signal(s->id,WRITE_WAIT,PCSL_NET_INTERRUPTED)) s->wait&=~WRITE_WAIT;
            if(!s->wait) memset(s,0,sizeof(*s));
            continue;
        }
        map[n]=s; fds[n].fd=s->fd;
        fds[n].events=((s->wait&READ_WAIT)?POLLIN:0)|((s->wait&WRITE_WAIT)?POLLOUT:0);
        fds[n++].revents=0;
    }
    if(n && poll(fds,n,0)>0) for(i=0;i<n;i++) {
        Socket *s=map[i]; int r=fds[i].revents;
        if((s->wait&READ_WAIT) && (r&(POLLIN|POLLERR|POLLHUP|POLLNVAL)))
            if(ps3me_network_signal(s->id,READ_WAIT,0)) s->wait&=~READ_WAIT;
        if((s->wait&WRITE_WAIT) && (r&(POLLOUT|POLLERR|POLLHUP|POLLNVAL)))
            if(ps3me_network_signal(s->id,WRITE_WAIT,0)) s->wait&=~WRITE_WAIT;
    }
    SDL_LockMutex(dns_lock);
    for(i=0;i<DNS_JOBS;i++) {
        Lookup *j=&lookups[i];
        if(j->id && j->done) {
            if(j->abandoned) reap(j);
            else if(!j->notified && ps3me_network_signal(j->id,LOOKUP_WAIT,j->result)) j->notified=1;
        }
    }
    SDL_UnlockMutex(dns_lock);
}
int pcsl_socket_open_start(unsigned char *ip,int port,void **h,void **ctx) {
    struct sockaddr_in a; Socket *s; int fd,r;
    if(!ip || !h || !ctx || port<1 || port>65535) return PCSL_NET_INVALID;
    *h=NULL; *ctx=NULL;
    if(pcsl_network_init()!=0) return PCSL_NET_IOERROR;
    fd=socket(AF_INET,SOCK_STREAM,0); if(fd<0) return failed();
    s=adopt(fd); if(!s) return PCSL_NET_IOERROR;
    address(&a,ip,port); r=connect(fd,(struct sockaddr*)&a,sizeof(a));
    if(r==0) { *h=handle(s); return 0; }
    last_error=NETERR;
    if(wouldblock()) { s->wait|=WRITE_WAIT; *h=handle(s); return PCSL_NET_WOULDBLOCK; }
    discard(s); return PCSL_NET_CONNECTION_NOTFOUND;
}
int pcsl_socket_open_finish(void *h,void *ctx) {
    Socket *s=find(h); struct pollfd p; int e=0; socklen_t size=sizeof(e); (void)ctx;
    if(!s) return PCSL_NET_INTERRUPTED;
    p.fd=s->fd; p.events=POLLOUT; p.revents=0;
    { int ready=poll(&p,1,0);
    if(ready<0) { last_error=NETERR; discard(s); return PCSL_NET_IOERROR; }
    if(ready==0) { s->wait|=WRITE_WAIT; return PCSL_NET_WOULDBLOCK; }
    }
    if(getsockopt(s->fd,SOL_SOCKET,SO_ERROR,&e,&size)<0) e=NETERR;
    if(e) { last_error=e; discard(s); return PCSL_NET_IOERROR; }
    s->wait&=~WRITE_WAIT; return 0;
}
static int transfer(void *h,void *data,int len,int *count,int writeop) {
    Socket *s=find(h); int r,flag=writeop?WRITE_WAIT:READ_WAIT;
    if(!s) return PCSL_NET_INTERRUPTED;
    if(len<0 || (!data && len) || !count) return PCSL_NET_INVALID;
    if(!len) { *count=0; return 0; }
    s->wait&=~flag;
#ifdef MSG_NOSIGNAL
    r=writeop?send(s->fd,data,len,MSG_NOSIGNAL):recv(s->fd,data,len,0);
#else
    r=writeop?send(s->fd,data,len,0):recv(s->fd,data,len,0);
#endif
    if(r>=0) { *count=r; return 0; }
    last_error=NETERR;
    if(wouldblock()) { s->wait|=flag; return PCSL_NET_WOULDBLOCK; }
    return PCSL_NET_IOERROR;
}
int pcsl_socket_read_start(void*h,unsigned char*d,int n,int*c,void**ctx) { *ctx=NULL; return transfer(h,d,n,c,0); }
int pcsl_socket_read_finish(void*h,unsigned char*d,int n,int*c,void*ctx) { (void)ctx; return transfer(h,d,n,c,0); }
int pcsl_socket_write_start(void*h,char*d,int n,int*c,void**ctx) { *ctx=NULL; return transfer(h,d,n,c,1); }
int pcsl_socket_write_finish(void*h,char*d,int n,int*c,void*ctx) { (void)ctx; return transfer(h,d,n,c,1); }
int pcsl_socket_close_start(void *h,void **ctx) {
    Socket *s=find(h); if(ctx) *ctx=NULL;
    if(!s) return PCSL_NET_INTERRUPTED;
    /* Retain pending wakeups when the UI event queue is full. */
    CLOSE(s->fd); s->fd=-1; s->wait=READ_WAIT|WRITE_WAIT;
    ps3me_network_poll(); return 0;
}
int pcsl_socket_close_finish(void *h,void *ctx) { (void)h; (void)ctx; return 0; }
int pcsl_socket_available(void *h,int *count) {
    Socket *s=find(h); if(!s || !count) return PCSL_NET_INVALID;
#ifdef PS3ME_PPU
    /* FIONREAD is not exposed by this PSL1GHT version. Non-consuming peek. */
    { char bytes[8192]; int r=recv(s->fd,bytes,sizeof(bytes),MSG_PEEK|MSG_DONTWAIT);
      if(r>=0) { *count=r; return 0; }
      if(wouldblock()) { *count=0; return 0; }
      return failed(); }
#else
    if(ioctl(s->fd,FIONREAD,count)<0) return failed(); return 0;
#endif
}
int pcsl_socket_shutdown_output(void *h) { Socket*s=find(h); return s && shutdown(s->fd,SHUT_WR)==0?0:PCSL_NET_IOERROR; }
static int endpoint(void *h,char *out,int *port,int remote) {
    Socket *s=find(h); struct sockaddr_in a; socklen_t n=sizeof(a); int r;
    if(!s) return PCSL_NET_INTERRUPTED;
    r=remote?getpeername(s->fd,(struct sockaddr*)&a,&n):getsockname(s->fd,(struct sockaddr*)&a,&n);
    if(r<0) return failed();
    if(out) { const unsigned char*b=(unsigned char*)&a.sin_addr; sprintf(out,"%u.%u.%u.%u",b[0],b[1],b[2],b[3]); }
    if(port) *port=ntohs(a.sin_port); return 0;
}
int pcsl_socket_getlocaladdr(void*h,char*out) { return endpoint(h,out,NULL,0); }
int pcsl_socket_getremoteaddr(void*h,char*out) { return endpoint(h,out,NULL,1); }
int pcsl_network_getlocalport(void*h,int*p) { return endpoint(h,NULL,p,0); }
int pcsl_network_getremoteport(void*h,int*p) { return endpoint(h,NULL,p,1); }
int pcsl_network_error(void*h) { (void)h; return last_error; }
int pcsl_network_getLocalHostName(char*out) { strcpy(out,"PS3ME"); return 0; }
int pcsl_network_getLocalIPAddressAsString(char*out) {
#ifdef PS3ME_PPU
    union net_ctl_info info;
    if(pcsl_network_init()!=0 || netCtlGetInfo(NET_CTL_INFO_IP_ADDRESS,&info)<0) return PCSL_NET_IOERROR;
    strcpy(out,info.ip_address); return 0;
#else
    strcpy(out,"127.0.0.1"); return 0;
#endif
}
int pcsl_network_getHostByAddr_start(int ip,char*out,void**h,void**ctx) {
    (void)ip; (void)out; (void)h; (void)ctx; return PCSL_NET_IOERROR;
}
int pcsl_network_getHostByAddr_finish(int ip,char*out,void**h,void*ctx) {
    (void)ip; (void)out; (void)h; (void)ctx; return PCSL_NET_IOERROR;
}
char *pcsl_inet_ntoa(void *ip) {
    static char text[16]; unsigned char*b=(unsigned char*)ip;
    sprintf(text,"%u.%u.%u.%u",b[0],b[1],b[2],b[3]); return text;
}
int pcsl_network_addrToString(unsigned char *ip,unsigned short **out,int *len) {
    char *s=pcsl_inet_ntoa(ip); int i,n=strlen(s);
    *out=(unsigned short*)pcsl_mem_malloc((n+1)*sizeof(unsigned short));
    if(!*out) return PCSL_NET_IOERROR;
    for(i=0;i<=n;i++) (*out)[i]=(unsigned char)s[i]; *len=n; return 0;
}
unsigned int pcsl_network_htonl(unsigned int n) { return htonl(n); }
unsigned int pcsl_network_ntohl(unsigned int n) { return ntohl(n); }
unsigned short pcsl_network_htons(unsigned short n) { return htons(n); }
unsigned short pcsl_network_ntohs(unsigned short n) { return ntohs(n); }
void pcsl_add_network_notifier(void*h,int event) {
    Socket*s=find(h); if(s) s->wait|=(event&PCSL_NET_CHECK_WRITE?WRITE_WAIT:READ_WAIT);
}
void pcsl_remove_network_notifier(void*h,int event) {
    Socket*s=find(h); if(s) s->wait&=~(event&PCSL_NET_CHECK_WRITE?WRITE_WAIT:READ_WAIT);
}
static int option(void*h,int flag,int *value,int set) {
    Socket*s=find(h); int level=SOL_SOCKET,opt; socklen_t size=sizeof(int); struct linger l;
    void *p=value;
    if(!s || flag<0 || flag>4 || (set && *value<0)) return PCSL_NET_INVALID;
    switch(flag) {
    case 0: level=IPPROTO_TCP; opt=TCP_NODELAY;
        if(set) *value=!*value; /* MIDP DELAY=0 disables Nagle. */
        break;
    case 1: opt=SO_LINGER; l.l_onoff=set && *value!=0; l.l_linger=set?*value:0; p=&l; size=sizeof(l); break;
    case 2: opt=SO_KEEPALIVE; break;
    case 3: opt=SO_RCVBUF; break;
    default: opt=SO_SNDBUF; break;
    }
    if((set?setsockopt(s->fd,level,opt,p,size):getsockopt(s->fd,level,opt,p,&size))<0) return failed();
    if(!set && flag==0) *value=!*value;
    if(!set && flag==1) *value=l.l_onoff?l.l_linger:0;
    return 0;
}
int pcsl_network_getsockopt(void*h,int flag,int*v) { return option(h,flag,v,0); }
int pcsl_network_setsockopt(void*h,int flag,int v) { return option(h,flag,&v,1); }
static int bind_socket(int port,void **h,int server) {
    int fd,on=1; Socket*s; struct sockaddr_in a;
    if(port<0 || port>65535 || !h) return PCSL_NET_INVALID;
    *h=NULL; if(pcsl_network_init()!=0) return PCSL_NET_IOERROR;
    fd=socket(AF_INET,server?SOCK_STREAM:SOCK_DGRAM,0); if(fd<0) return failed();
    s=adopt(fd); if(!s) return PCSL_NET_IOERROR;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    address(&a,NULL,port);
    if(bind(fd,(struct sockaddr*)&a,sizeof(a))<0 || (server && listen(fd,3)<0)) { last_error=NETERR; discard(s); return PCSL_NET_IOERROR; }
    *h=handle(s); return 0;
}
int pcsl_serversocket_open(int port,void**h) { return bind_socket(port,h,1); }
int pcsl_server_socket_open_start(int port,void**h,void**ctx) { *ctx=NULL; return bind_socket(port,h,1); }
int pcsl_server_socket_open_finish(void**h,void**ctx) { (void)h; (void)ctx; return PCSL_NET_INVALID; }
int pcsl_serversocket_accept_start(void*h,void**out,void**ctx) {
    Socket*s=find(h),*client; int fd; *ctx=NULL;
    if(!s) return PCSL_NET_INTERRUPTED;
    s->wait&=~READ_WAIT; fd=accept(s->fd,NULL,NULL);
    if(fd<0) { if(wouldblock()) { s->wait|=READ_WAIT; return PCSL_NET_WOULDBLOCK; } return failed(); }
    client=adopt(fd); if(!client) return PCSL_NET_IOERROR;
    *out=handle(client); return 0;
}
int pcsl_serversocket_accept_finish(void*h,void**out,void**ctx) { return pcsl_serversocket_accept_start(h,out,ctx); }
int pcsl_serversocket_close_start(void*h,void**ctx) { return pcsl_socket_close_start(h,ctx); }
int pcsl_serversocket_close_finish(void*h,void*ctx) { return pcsl_socket_close_finish(h,ctx); }
int pcsl_datagram_open_start(int port,void**h,void**ctx) { *ctx=NULL; return bind_socket(port,h,0); }
int pcsl_datagram_open_finish(void*h,void*ctx) { (void)h; (void)ctx; return PCSL_NET_INVALID; }
static int datagram(void*h,unsigned char*ip,int *port,char*data,int n,int*count,int writing) {
    Socket*s=find(h); struct sockaddr_in a; socklen_t size=sizeof(a); int r,flag=writing?WRITE_WAIT:READ_WAIT;
    if(!s) return PCSL_NET_INTERRUPTED;
    if(!ip || !port || !count || n<0 || (!data && n) || (writing && (*port<1 || *port>65535))) return PCSL_NET_INVALID;
    s->wait&=~flag; address(&a,writing?ip:NULL,writing?*port:0);
    r=writing?sendto(s->fd,data,n,0,(struct sockaddr*)&a,size):recvfrom(s->fd,data,n,0,(struct sockaddr*)&a,&size);
    if(r<0) { if(wouldblock()) { s->wait|=flag; return PCSL_NET_WOULDBLOCK; } return failed(); }
    *count=r; if(!writing) { memcpy(ip,&a.sin_addr,4); *port=ntohs(a.sin_port); } return 0;
}
int pcsl_datagram_read_start(void*h,unsigned char*ip,int*p,char*d,int n,int*c,void**ctx) { *ctx=NULL; return datagram(h,ip,p,d,n,c,0); }
int pcsl_datagram_read_finish(void*h,unsigned char*ip,int*p,char*d,int n,int*c,void*ctx) { (void)ctx; return datagram(h,ip,p,d,n,c,0); }
int pcsl_datagram_write_start(void*h,unsigned char*ip,int p,char*d,int n,int*c,void**ctx) { *ctx=NULL; return datagram(h,ip,&p,d,n,c,1); }
int pcsl_datagram_write_finish(void*h,unsigned char*ip,int p,char*d,int n,int*c,void*ctx) { (void)ctx; return datagram(h,ip,&p,d,n,c,1); }
int pcsl_datagram_close_start(void*h,void**ctx) { return pcsl_socket_close_start(h,ctx); }
int pcsl_datagram_close_finish(void*h,void*ctx) { return pcsl_socket_close_finish(h,ctx); }
