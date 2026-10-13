/* Invented finite protocol replies; no firmware/peripheral calls. */
#include "netsentinel/wifi_discovery.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void feed(NsWifiDiscovery* d,const char* text,uint32_t now) {
    for(size_t i=0;i<strlen(text);i++) assert(ns_wifi_discovery_feed(d,(const uint8_t*)text+i,1,now)==NS_OK);
}
static void reading(NsWifiDiscovery* d,uint32_t now) {
    ns_wifi_discovery_init(d);
    assert(ns_wifi_discovery_begin(d,now,1000,10)==NS_OK);
    assert(ns_wifi_discovery_begin(d,now,1000,10)==NS_BUSY);
    feed(d,"#scanap\r\nStarting AP scan. Stop with stopscan\r\n> ",now);
    assert(d->state==NS_WIFI_OBSERVING && d->peer_may_scan);
    assert(ns_wifi_discovery_line(d,"-66 Ch: 6 02:00:00:00:00:01 ESSID: Synthetic",now)==NS_OK && !d->count);
    assert(ns_wifi_discovery_tick(d,now+10)==NS_OK && d->intent==NS_WIFI_INTENT_STOP);
    feed(d,"#stopscan\nStopping WiFi tran/recv\n> ",now+11);
    assert(!d->peer_may_scan && d->state==NS_WIFI_READING && d->intent==NS_WIFI_INTENT_LIST);
}
static void parsers(void) {
    NsWifiRecord record={0},before;
    assert(ns_wifi_parse_list("[0][CH:6] Synthetic Lab -66",&record)==NS_OK);
    assert(!strcmp(record.ap.ssid,"Synthetic Lab") && record.ap.channel==6 && record.ap.rssi==-66);
    assert(!record.bssid_reported && record.security==NS_WIFI_SECURITY_UNREPORTED && !record.ap.bssid[0]);
    assert(ns_wifi_parse_list("[1][CH:1]  -90 (selected)",&record)==NS_OK && !record.ap.ssid[0]);
    const char* invalid[]={"[0][CH:3x] Lab -50","[0][CH:3] Lab -50x","[-1][CH:3] Lab -50",
        "[0][CH:0] Lab -50","[0][CH:3] Lab","[0][CH:3] Lab -9999"};
    before=record;
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++) {
        assert(ns_wifi_parse_list(invalid[i],&record)==NS_INVALID && !memcmp(&before,&record,sizeof(record)));
    }
    assert(ns_wifi_parse_list("[0][CH:3] 12345678901234567890123456789012345 -50",&record)==NS_FULL);
    const char* detail="   ESSID: Synthetic Lab\n   BSSID: 02:00:00:00:00:01\n Channel: 6\n    RSSI: -66\nSecurity: WPA2\n";
    assert(ns_wifi_parse_detail(detail,strlen(detail),&record)==NS_OK);
    assert(record.bssid_reported && record.security==NS_WIFI_SECURITY_WPA2);
    before=record;
    assert(ns_wifi_parse_detail("ESSID: Lab\nRSSI: -40\n",21,&record)==NS_INVALID);
    assert(!memcmp(&before,&record,sizeof(record)));
    char duplicate[512];snprintf(duplicate,sizeof(duplicate),"%sSecurity: WPA2\n",detail);
    assert(ns_wifi_parse_detail(duplicate,strlen(duplicate),&record)==NS_INVALID);
    char unknown[512];strcpy(unknown,detail);char* p=strstr(unknown,"WPA2");strcpy(p,"Alien\n");
    assert(ns_wifi_parse_detail(unknown,strlen(unknown),&record)==NS_UNSUPPORTED);
    for(unsigned i=0;i<10000;i++) {
        char text[NS_WIFI_LINE_BYTES]; snprintf(text,sizeof(text),"[%u][CH:%u] Synthetic %u %d",i%48,1+i%14,i%10,-20-(int)(i%100));
        assert(ns_wifi_parse_list(text,&record)==NS_OK && record.ap.channel>=1 && record.ap.channel<=14);
    }
}
static void sessions(void) {
    NsWifiDiscovery d;
    for(unsigned i=0;i<12;i++) {
        uint32_t now=UINT32_MAX-20u+i*100u;reading(&d,now);
        feed(&d,"#list -a\n[0][CH:6] Synthetic Lab -66\n[0][CH:6] Synthetic Lab -64\n0 selected\n> ",now+12);
        assert(d.state==NS_WIFI_COMPLETE && d.count==1 && d.records[0].ap.rssi==-64 && !d.peer_may_scan);
        assert(ns_wifi_discovery_cancel(&d,now+13)==NS_CANCELLED && !d.peer_may_scan);
    }
    reading(&d,0);feed(&d,"#list -a\n0 selected\n> ",12);
    assert(d.state==NS_WIFI_EMPTY && !d.count);
    reading(&d,0);assert(ns_wifi_discovery_line(&d,"#list -a",12)==NS_OK);
    assert(ns_wifi_discovery_line(&d,"> ",12)==NS_INVALID); /* Empty requires summary. */
    reading(&d,0);feed(&d,"#list -a\n",12);
    assert(ns_wifi_discovery_line(&d,"[0][CH:6] Lab -66",12)==NS_OK);
    assert(ns_wifi_discovery_line(&d,"[0][CH:7] Other -66",12)==NS_INVALID);
    ns_wifi_discovery_init(&d);assert(ns_wifi_discovery_begin(&d,0,1000,10)==NS_OK);
    assert(ns_wifi_discovery_cancel(&d,1)==NS_BUSY && d.peer_may_scan);
    assert(ns_wifi_discovery_cancel(&d,500)==NS_BUSY && d.stage_ms==1);
    assert(ns_wifi_discovery_tick(&d,1001)==NS_TIMEOUT && d.peer_may_scan);
    assert(ns_wifi_discovery_begin(&d,1002,1000,10)==NS_BUSY); /* Host timeout is not peer stop. */
    assert(ns_wifi_discovery_cancel(&d,1003)==NS_BUSY);
    assert(ns_wifi_discovery_line(&d,"#stopscan",1004)==NS_OK);
    assert(ns_wifi_discovery_line(&d,"Stopping WiFi tran/recv",1004)==NS_OK);
    assert(ns_wifi_discovery_line(&d,"> ",1004)==NS_CANCELLED && !d.peer_may_scan);
    reading(&d,0);feed(&d,"#list -a\n",12);
    uint8_t bad[]={0};assert(ns_wifi_discovery_feed(&d,bad,1,12)==NS_INVALID);
    reading(&d,0);feed(&d,"#list -a\n",12);
    char long_line[NS_WIFI_LINE_BYTES+1];memset(long_line,'x',sizeof(long_line));long_line[sizeof(long_line)-1]=0;
    assert(ns_wifi_discovery_line(&d,long_line,12)==NS_FULL);
    reading(&d,0);assert(ns_wifi_discovery_tick(&d,1011)==NS_TIMEOUT && !d.peer_may_scan);
    reading(&d,0);feed(&d,"#list -a\n",12);
    for(unsigned i=0;i<NS_WIFI_RESULTS;i++) {
        char line[80];snprintf(line,sizeof(line),"[%u][CH:6] Synthetic%u -66",i,i);
        assert(ns_wifi_discovery_line(&d,line,12)==NS_OK);
    }
    assert(ns_wifi_discovery_line(&d,"[48][CH:6] Overflow -66",12)==NS_FULL && !d.results_valid);
    reading(&d,0);feed(&d,"#list -a\n",12);
    d.total_bytes=NS_WIFI_REPLY_BYTES;
    assert(ns_wifi_discovery_line(&d,"",12)==NS_FULL);
}
int main(void) { parsers();sessions();puts("PASS: host-only Wi-Fi result parser, bounded protocol intents, cancellation and peer cleanup uncertainty"); }
