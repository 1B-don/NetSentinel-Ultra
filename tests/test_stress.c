/* Deterministic synthetic fuzz/stress, never hardware or captured UART data. */
#include "netsentinel/application.h"
#include "mock_platform.h"
#include "ns_analysis.h"
#include <stdio.h>
#include <stdlib.h>
static uint32_t state=0x4e535531;
static uint32_t next(void) { state ^= state<<13; state ^= state>>17; state ^= state<<5; return state; }
static void strings(void) {
    for(unsigned run=0;run<20000;run++) {
        size_t n=next()%512; char* text=malloc(n+1);assert(text);
        for(size_t i=0;i<n;i++) text[i]=(char)(next()%128);
        text[n]=0;
        NsSettings before=ns_settings_default(), result=before;
        NsResult r=ns_settings_parse(text,&result);
        if(r!=NS_OK) assert(!memcmp(&before,&result,sizeof(result)));
        else { char formatted[100];assert(ns_settings_format(&result,formatted,sizeof(formatted))==NS_OK); }
        NsAccessPoint ap;
        if(ns_parse_ap_line(text,&ap)) {
            assert(ap.channel>=1&&ap.channel<=14&&ap.rssi>=-127&&ap.rssi<=20);
            assert(memchr(ap.ssid,0,sizeof(ap.ssid))&&memchr(ap.bssid,0,sizeof(ap.bssid)));
            char csv[128];assert(ns_csv_ap(&ap,csv,sizeof(csv)));
            assert(strstr(csv,"redacted")&&!strstr(csv,":"));
        }
        free(text);
    }
}
static void uart_stress(void) {
    for(unsigned run=0;run<2000;run++) {
        Mock m={0}; NsPlatform io=platform(&m);NsSession s;ns_session_init(&s,&io,NULL);
        s.use_marauder=true;uint32_t start=UINT32_MAX-(next()%100);
        assert(ns_session_start(&s,NS_RADIO_ESP,false,false,start)==NS_OK);
        assert(ns_session_feed(&s,(const uint8_t*)"#info\n",6,start)==NS_OK);
        uint8_t bytes[64];
        for(unsigned chunk=0;chunk<8&&s.active;chunk++) {
            size_t n=next()%sizeof(bytes);
            for(size_t i=0;i<n;i++) bytes[i]=(uint8_t)next();
            (void)ns_session_feed(&s,bytes,n,start+chunk);
        }
        if(s.active) {
            if(run%2) assert(ns_session_cancel(&s,start+10)==NS_OK);
            else assert(ns_session_tick(&s,start+5000)==NS_TIMEOUT);
        }
        assert(!s.active&&!m.owner&&m.acquired==m.released);
        assert(!strcmp(m.commands,"info\n"));
        for(size_t i=0;i<sizeof(s.line);i++) assert(!s.line[i]);
    }
}
static void radio_and_navigation(void) {
    for(unsigned run=0;run<200;run++) {
        Mock m={0};NsPlatform io=platform(&m);NsNrf n={0};
        assert(ns_nrf_open(&n,&io,true)==NS_OK);
        for(unsigned ch=0;ch<NS_NRF_CHANNELS;ch++) {
            m.regs[9]=(uint8_t)(next()&1);
            assert(ns_nrf_sample(&n,(uint8_t)ch,200)==NS_OK);
        }
        NsActivitySummary s;assert(ns_activity_summary(&n,&s)==NS_OK&&s.measured_channels==126&&s.samples==126);
        assert(ns_nrf_close(&n)==NS_OK&&!m.owner&&!m.ce&&m.acquired==m.released);
    }
    NsApplication a={0};a.running=true;a.observation_filter=ns_observation_filter_default();ns_saved_defaults(&a.saved);
    for(unsigned i=0;i<10000;i++) {
        a.running=true;(void)ns_app_key(&a,(NsKey)(next()%10),next());
        NsFrame frame;ns_app_frame(&a,&frame);
        assert(a.menu<9&&a.filter_setting<3&&a.info_view<3);
        for(unsigned row=0;row<6;row++) assert(memchr(frame.rows[row],0,sizeof(frame.rows[row])));
    }
}
int main(void) {
    strings();uart_stress();radio_and_navigation();
    puts("PASS: 20000 parser mutations, 2000 interrupted/wrapped UART sessions, 25200 PRX samples and 10000 UI events (MOCK)");
}
