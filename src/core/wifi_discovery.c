#include "../../include/netsentinel/wifi_discovery.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
static const char* const securities[] = {"Not reported","Open","WEP","WPA","WPA2","WPA3",
    "WPA/WPA2 Mixed","WPA2 Enterprise","WPA3 Enterprise","WAPI","Unknown"};
const char* ns_wifi_security_name(NsWifiSecurity s) {
    return (unsigned)s < sizeof(securities)/sizeof(*securities) ? securities[s] : "Unknown";
}
static bool printable(const char* p, size_t n) {
    for(size_t i=0;i<n;i++) if((unsigned char)p[i]<32 || (unsigned char)p[i]>126) return false;
    return true;
}
static bool number(const char** p, long* value, bool signed_value) {
    const char* start=*p;
    if(signed_value && **p=='-') (*p)++;
    if(!isdigit((unsigned char)**p)) return false;
    unsigned digits=0;
    while(isdigit((unsigned char)**p)) { if(++digits>6) return false; (*p)++; }
    *value=strtol(start,NULL,10); return true;
}
NsResult ns_wifi_parse_list(const char* line, NsWifiRecord* out) {
    if(!line || !out) return NS_INVALID;
    size_t length=0; while(length<NS_WIFI_LINE_BYTES && line[length]) length++;
    if(length==NS_WIFI_LINE_BYTES) return NS_FULL;
    if(!printable(line,length)) return NS_INVALID;
    const char* p=line; long index,channel,rssi;
    if(*p++!='[' || !number(&p,&index,false) || strncmp(p,"][CH:",5)) return NS_INVALID;
    p+=5;
    if(index>65535 || !number(&p,&channel,false) || strncmp(p,"] ",2)) return NS_INVALID;
    p+=2; const char* name=p; const char* end=line+length;
    const char suffix[]=" (selected)";
    if((size_t)(end-name)>=sizeof(suffix)-1 && !memcmp(end-(sizeof(suffix)-1),suffix,sizeof(suffix)-1)) end-=sizeof(suffix)-1;
    const char* token=end; while(token>name && token[-1]!=' ') token--;
    if(token==name) return NS_INVALID;
    const char* q=token;
    if(!number(&q,&rssi,true) || q!=end || channel<1 || channel>14 || rssi < -127 || rssi > 20) return NS_INVALID;
    size_t n=(size_t)(token-name)-1;
    if(n>32) return NS_FULL;
    NsWifiRecord candidate={0}; candidate.index=(uint16_t)index;
    memcpy(candidate.ap.ssid,name,n); candidate.ap.ssid[n]=0;
    candidate.ap.channel=(uint8_t)channel; candidate.ap.rssi=(int16_t)rssi;
    *out=candidate; return NS_OK;
}
static bool mac(const char* p) {
    if(strlen(p)!=17) return false;
    for(unsigned i=0;i<17;i++) if(i%3==2 ? p[i]!=':' : !isxdigit((unsigned char)p[i])) return false;
    return true;
}
NsResult ns_wifi_parse_detail(const char* text, size_t bytes, NsWifiRecord* out) {
    if(!text || !out || !bytes) return NS_INVALID;
    if(bytes>1024) return NS_FULL;
    NsWifiRecord candidate={0}; unsigned fields=0;
    size_t offset=0;
    while(offset<bytes) {
        char line[NS_WIFI_LINE_BYTES]; size_t n=0;
        while(offset<bytes && text[offset]!='\n') {
            if(n+1==sizeof(line)) return NS_FULL;
            line[n++]=text[offset++];
        }
        if(offset<bytes) offset++;
        if(n && line[n-1]=='\r') n--;
        if(!printable(line,n)) return NS_INVALID;
        line[n]=0; const char* p=line; while(*p==' ') p++;
        const char* labels[]={"ESSID: ","BSSID: ","Channel: ","RSSI: ","Security: "};
        unsigned field=5;
        for(unsigned i=0;i<5;i++) if(!strncmp(p,labels[i],strlen(labels[i]))) { field=i; p+=strlen(labels[i]); break; }
        if(field==5) continue; /* Ignore non-result fields, never retain. */
        if(fields&(1u<<field)) return NS_INVALID;
        fields|=1u<<field;
        if(field==0) {
            if(strlen(p)>32) return NS_FULL;
            strcpy(candidate.ap.ssid,p);
        } else if(field==1) {
            if(!mac(p)) return NS_INVALID;
            for(unsigned i=0;i<17;i++) candidate.ap.bssid[i]=(char)tolower((unsigned char)p[i]);
            candidate.bssid_reported=true;
        } else if(field==2 || field==3) {
            long value; const char* q=p;
            if(!number(&q,&value,field==3) || *q) return NS_INVALID;
            if(field==2) { if(value<1 || value>14) return NS_INVALID; candidate.ap.channel=(uint8_t)value; }
            else { if(value < -127 || value > 20) return NS_INVALID; candidate.ap.rssi=(int16_t)value; }
        } else {
            unsigned i=1; for(;i<sizeof(securities)/sizeof(*securities);i++) if(!strcmp(p,securities[i])) break;
            if(i==sizeof(securities)/sizeof(*securities)) return NS_UNSUPPORTED;
            candidate.security=(NsWifiSecurity)i;
            strcpy(candidate.ap.security,p);
        }
    }
    if(fields!=31u) return NS_INVALID;
    *out=candidate; return NS_OK;
}
void ns_wifi_discovery_init(NsWifiDiscovery* d) { if(d) memset(d,0,sizeof(*d)); }
static NsResult failed(NsWifiDiscovery* d,NsResult r) {
    memset(d->line,0,sizeof(d->line)); d->line_len=0; d->pending_cr=false;
    d->state=r==NS_TIMEOUT ? NS_WIFI_TIMEOUT : NS_WIFI_ERROR;
    d->intent=NS_WIFI_INTENT_NONE; return d->result=r;
}
NsResult ns_wifi_discovery_begin(NsWifiDiscovery* d,uint32_t now,uint32_t timeout,uint32_t dwell) {
    if(!d || timeout<1000 || timeout>60000 || !dwell || dwell>60000) return NS_INVALID;
    if(d->peer_may_scan || d->state==NS_WIFI_READING) return NS_BUSY;
#ifndef NS_TEST_WIFI_DISCOVERY_PROTOCOL
    (void)now; d->state=NS_WIFI_UNAVAILABLE; d->result=NS_UNSUPPORTED; return NS_UNSUPPORTED;
#else
    ns_wifi_discovery_init(d); d->stage_ms=now; d->timeout_ms=timeout; d->dwell_ms=dwell;
    d->state=NS_WIFI_STARTING; d->intent=NS_WIFI_INTENT_START;
    d->peer_may_scan=true; /* A dispatched start may outlive the host. */
    return NS_OK;
#endif
}
NsResult ns_wifi_discovery_tick(NsWifiDiscovery* d,uint32_t now) {
    if(!d) return NS_INVALID;
    if(d->state==NS_WIFI_OBSERVING && (uint32_t)(now-d->stage_ms)>=d->dwell_ms) {
        d->state=NS_WIFI_STOPPING; d->intent=NS_WIFI_INTENT_STOP; d->stage_ms=now; d->echo=d->ack=false;
    } else if((d->state==NS_WIFI_STARTING || d->state==NS_WIFI_STOPPING || d->state==NS_WIFI_READING) &&
        (uint32_t)(now-d->stage_ms)>=d->timeout_ms) return failed(d,NS_TIMEOUT);
    return d->result;
}
NsResult ns_wifi_discovery_cancel(NsWifiDiscovery* d,uint32_t now) {
    if(!d) return NS_INVALID;
    if(d->peer_may_scan) {
        if(d->state!=NS_WIFI_STOPPING) {
            d->state=NS_WIFI_STOPPING; d->intent=NS_WIFI_INTENT_STOP; d->stage_ms=now; d->echo=d->ack=false;
            d->result=NS_OK;
        }
        d->cancel_requested=true; return NS_BUSY;
    }
    memset(d->line,0,sizeof(d->line)); d->line_len=0; d->pending_cr=false;
    d->state=NS_WIFI_CANCELLED; d->result=NS_CANCELLED; d->intent=NS_WIFI_INTENT_NONE; return NS_CANCELLED;
}
NsResult ns_wifi_discovery_line(NsWifiDiscovery* d,const char* line,uint32_t now) {
    if(!d || !line) return NS_INVALID;
    NsResult r=ns_wifi_discovery_tick(d,now); if(r!=NS_OK) return r;
    if(d->state!=NS_WIFI_STARTING && d->state!=NS_WIFI_STOPPING && d->state!=NS_WIFI_READING && d->state!=NS_WIFI_OBSERVING) return NS_UNSUPPORTED;
    size_t n=0; while(n<NS_WIFI_LINE_BYTES && line[n]) n++;
    if(n==NS_WIFI_LINE_BYTES || d->total_bytes>NS_WIFI_REPLY_BYTES-n-1u) return failed(d,NS_FULL);
    d->total_bytes+=(uint32_t)n+1u;
    if(!printable(line,n)) return failed(d,NS_INVALID);
    if(d->state==NS_WIFI_OBSERVING) return NS_OK; /* Drain stream; collect only bounded list reply. */
    const char* echo=d->state==NS_WIFI_STARTING ? "#scanap" : d->state==NS_WIFI_STOPPING ? "#stopscan" : "#list -a";
    if(!strcmp(line,echo)) { if(d->echo) return failed(d,NS_INVALID); d->echo=true; d->intent=NS_WIFI_INTENT_NONE; return NS_OK; }
    if(!d->echo) return NS_OK;
    if(!strcmp(line,"> ")) {
        if(d->state!=NS_WIFI_READING && !d->ack) return failed(d,NS_INVALID);
        if(d->state==NS_WIFI_READING && !d->summary) return failed(d,NS_INVALID);
        d->echo=d->ack=false; d->stage_ms=now;
        if(d->state==NS_WIFI_STARTING) d->state=NS_WIFI_OBSERVING;
        else if(d->state==NS_WIFI_STOPPING) {
            d->peer_may_scan=false;
            if(d->cancel_requested) { d->state=NS_WIFI_CANCELLED; return d->result=NS_CANCELLED; }
            d->state=NS_WIFI_READING; d->intent=NS_WIFI_INTENT_LIST;
        } else { d->state=d->count ? NS_WIFI_COMPLETE : NS_WIFI_EMPTY; d->results_valid=true; }
        return NS_OK;
    }
    if(d->state==NS_WIFI_STARTING) d->ack|=!strcmp(line,"Starting AP scan. Stop with stopscan");
    else if(d->state==NS_WIFI_STOPPING) d->ack|=!strcmp(line,"Stopping WiFi tran/recv");
    else if(*line=='[') {
        if(d->summary) return failed(d,NS_INVALID);
        NsWifiRecord record; r=ns_wifi_parse_list(line,&record); if(r!=NS_OK) return failed(d,r);
        if(record.index>=NS_WIFI_RESULTS) return failed(d,NS_FULL);
        for(size_t i=0;i<d->count;i++) if(d->records[i].index==record.index) {
            if(strcmp(d->records[i].ap.ssid,record.ap.ssid) || d->records[i].ap.channel!=record.ap.channel) return failed(d,NS_INVALID);
            d->records[i]=record; return NS_OK;
        }
        if(d->count==NS_WIFI_RESULTS) return failed(d,NS_FULL);
        d->records[d->count++]=record;
    } else if(d->state==NS_WIFI_READING && *line) {
        const char* p=line; long selected;
        if(d->summary || !number(&p,&selected,false) || strcmp(p," selected") || selected>(long)d->count) return failed(d,NS_INVALID);
        d->summary=true;
    }
    return NS_OK;
}
NsResult ns_wifi_discovery_feed(NsWifiDiscovery* d,const uint8_t* bytes,size_t count,uint32_t now) {
    if(!d || (!bytes && count)) return NS_INVALID;
    if(count>1024) return failed(d,NS_FULL);
    NsResult r=ns_wifi_discovery_tick(d,now); if(r!=NS_OK) return r;
    if(d->state!=NS_WIFI_STARTING && d->state!=NS_WIFI_STOPPING && d->state!=NS_WIFI_READING && d->state!=NS_WIFI_OBSERVING) return NS_UNSUPPORTED;
    for(size_t i=0;i<count;i++) {
        uint8_t c=bytes[i];
        if(d->pending_cr && c!='\n') return failed(d,NS_INVALID);
        if(c=='\r') { d->pending_cr=true; continue; }
        if(c=='\n' || (c==' ' && d->line_len==1 && d->line[0]=='>')) {
            if(c==' ') d->line[d->line_len++]=' ';
            d->line[d->line_len]=0;
            r=ns_wifi_discovery_line(d,d->line,now);
            memset(d->line,0,sizeof(d->line)); d->line_len=0; d->pending_cr=false;
            if(r!=NS_OK) return r;
            if(d->state==NS_WIFI_OBSERVING || d->state==NS_WIFI_COMPLETE || d->state==NS_WIFI_EMPTY) return NS_OK;
        } else {
            if(c<32 || c>126) return failed(d,NS_INVALID);
            if(d->line_len+1>=sizeof(d->line)) return failed(d,NS_FULL);
            d->line[d->line_len++]=(char)c;
        }
    }
    return NS_OK;
}
