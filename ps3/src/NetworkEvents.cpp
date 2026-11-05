// SPDX-License-Identifier: GPL-2.0-only
#include <cstdint>
#include <cstdio>
extern "C" {
#include <midpServices.h>
#include <midp_jc_event_defs.h>
#include <javacall_events.h>
int ps3me_network_signal(int handle,int kind,int status) {
    midp_jc_event_union event = {};
    event.eventType=MIDP_JC_EVENT_SOCKET;
    event.data.socketEvent.handle=(javacall_handle)(intptr_t)handle;
    event.data.socketEvent.waitingFor=kind==1?NETWORK_READ_SIGNAL:
        (kind==2?NETWORK_WRITE_SIGNAL:HOST_NAME_LOOKUP_SIGNAL);
    event.data.socketEvent.status=(javacall_result)status;
    int queued=javacall_event_send((unsigned char*)&event,sizeof(event))==JAVACALL_OK;
    if(queued && kind==2)
        std::fprintf(stderr,"[PS3ME net] write-ready queued handle=%d status=%d\n",handle,status);
    return queued;
}
}
