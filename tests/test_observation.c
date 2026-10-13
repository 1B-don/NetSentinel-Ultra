/* Independent synthetic observation/history inputs; no device captures. */
#include "netsentinel/application.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    NsAccessPoint aps[3] = {{.channel=1,.rssi=-60},{.channel=6,.rssi=-40},{.channel=1,.rssi=-90}};
    uint32_t seen[3] = {UINT32_MAX-10,10,0}; uint8_t indices[3] = {99,99,99}; size_t n = 99;
    NsObservationFilter f = ns_observation_filter_default();
    assert(ns_observation_select(aps,seen,3,&f,20,indices,3,&n)==NS_OK && n==3);
    f.channel=1; f.minimum_rssi=-70; f.maximum_age_ms=40;
    assert(ns_observation_select(aps,seen,3,&f,20,indices,3,&n)==NS_OK && n==1 && indices[0]==0);
    f.maximum_age_ms=30;
    assert(ns_observation_select(aps,seen,3,&f,20,indices,3,&n)==NS_OK && n==0);
    f=ns_observation_filter_default(); n=99;indices[0]=99;
    assert(ns_observation_select(aps,seen,3,&f,0,indices,1,&n)==NS_FULL && n==99 && indices[0]==99);
    f.channel=15; assert(ns_observation_select(aps,seen,3,&f,0,indices,3,&n)==NS_INVALID);
    f=ns_observation_filter_default(); aps[0].channel=0;
    assert(ns_observation_select(aps,seen,3,&f,0,indices,3,&n)==NS_OK && n==2);
    NsSavedState s={0}; s.history_count=3;
    s.history[0]=(NsHistory){.id=1,.radio=NS_RADIO_ESP,.result=NS_OK};
    s.history[1]=(NsHistory){.id=2,.radio=NS_RADIO_NRF,.result=NS_IO};
    s.history[2]=(NsHistory){.id=3,.radio=NS_RADIO_ESP,.result=NS_TIMEOUT};
    NsHistoryFilter hf={0}; assert(ns_history_select(&s,&hf,0)->id==3);
    hf.radio=NS_RADIO_NRF; assert(ns_history_select(&s,&hf,0)->id==2 && !ns_history_select(&s,&hf,1));
    hf.radio=NS_RADIO_ESP;hf.failures_only=true;assert(ns_history_select(&s,&hf,0)->id==3 && !ns_history_select(&s,&hf,1));
    NsNrf radio={0}; NsActivitySummary sum;
    radio.samples[5]=4;radio.hits[5]=3;radio.samples[8]=2;
    assert(ns_activity_summary(&radio,&sum)==NS_OK && sum.measured_channels==2 && sum.samples==6 && sum.hits==3);
    assert(sum.peak_channel==5 && sum.peak_percent==75 && sum.quiet_channel==8 && !sum.quiet_percent);
    radio.hits[0]=1;assert(ns_activity_summary(&radio,&sum)==NS_INVALID);
    NsApplication a={0}; a.running=true;a.page=NS_PAGE_FILTERS;a.observation_filter=f;
    assert(ns_app_key(&a,NS_KEY_RIGHT,0)==NS_OK && a.observation_filter.channel==1);
    assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK && a.observation_filter.channel==0);
    a.saved=s;a.page=NS_PAGE_HISTORY;
    assert(ns_app_key(&a,NS_KEY_RIGHT,0)==NS_OK && a.history_filter.radio==NS_RADIO_ESP);
    assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK && a.history_filter.failures_only);
    NsFrame frame;ns_app_frame(&a,&frame);assert(strstr(frame.rows[1],"Session 3"));
    a.page=NS_PAGE_RADIO;a.selected=NS_RADIO_ESP;a.session.ap_count=3;
    memcpy(a.session.aps,aps,sizeof(aps));strcpy(a.session.aps[1].ssid,"PRIVATE_SYNTHETIC_NAME");
    a.observation_filter.channel=6;ns_app_frame(&a,&frame);
    assert(strstr(frame.rows[1],"AP 2") && !strstr(frame.rows[1],"PRIVATE"));
    a.observation_filter.channel=14;ns_app_frame(&a,&frame);assert(strstr(frame.rows[1],"No observations"));
    puts("PASS: bounded filters, age wrap, history selection, RPD summaries and private UI (MOCK)");
}
