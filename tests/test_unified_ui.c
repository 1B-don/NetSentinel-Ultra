#include "ns_flipper.h"
#include "mock_sdk.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void go(NsApplication* a,unsigned index) {
    a->hub_index=index;assert(a->page==NS_PAGE_HUB);
    assert(ns_app_key(a,NS_KEY_OK,1)==NS_OK);
}
int main(void) {
    mock_sdk_reset(); NsFlipper f; NsApplication a;assert(ns_flipper_init(&f));
    assert(ns_app_init(&a,&f.platform,&f.store,f.capabilities)==NS_ABSENT);
    assert(a.unified_ui && a.page==NS_PAGE_HUB);
    for(unsigned cycle=0;cycle<3;cycle++) for(unsigned i=0;i<7;i++) {
        go(&a,i);NsFrame frame;ns_app_frame(&a,&frame);
        if(i==0) {
            assert(a.page==NS_PAGE_RADIO && a.selected==NS_RADIO_ESP);
            assert(ns_app_key(&a,NS_KEY_OK,2)==NS_OK && a.info_pending);
        } else if(i==1) {
            assert(ns_app_key(&a,NS_KEY_OK,2)==NS_UNSUPPORTED && !f.serial);
            assert(a.wifi_discovery.state==NS_WIFI_UNAVAILABLE && !a.wifi_discovery.peer_may_scan);
            assert(ns_app_key(&a,NS_KEY_RIGHT,3)==NS_OK && a.page==NS_PAGE_NETWORK_RESULTS);
            assert(ns_app_key(&a,NS_KEY_OK,3)==NS_OK && a.wifi_parser_checked && !a.wifi_discovery.count && !f.serial);
        } else if(i==6) {
            assert(a.page==NS_PAGE_HOME);
            a.menu=2;assert(ns_app_key(&a,NS_KEY_OK,2)==NS_OK && a.selected==NS_RADIO_NRF);
            assert(ns_app_key(&a,NS_KEY_OK,2)==NS_UNSUPPORTED);
        }
        assert(ns_app_key(&a,NS_KEY_BACK,4)==NS_OK || a.status==NS_UNSUPPORTED);
        assert(a.page==NS_PAGE_HUB && !f.serial && !a.info_pending && !f.owner);
    }
    go(&a,0);assert(ns_app_device_info(&a,100)==NS_OK && f.serial);
    a.page=NS_PAGE_HUB;go(&a,1);assert(ns_app_key(&a,NS_KEY_OK,101)==NS_BUSY);
    assert(ns_app_key(&a,NS_KEY_BACK,102)==NS_OK && !f.serial);
    const char* denied[]={"scanap\n","list -a\n","info -a 0\n","stopscan\n","sniffbeacon\n"};
    assert(f.platform.acquire(&f,NS_RADIO_ESP)==NS_OK);
    for(unsigned i=0;i<sizeof(denied)/sizeof(*denied);i++) assert(f.platform.uart_write(&f,(const uint8_t*)denied[i],strlen(denied[i]))==NS_UNSUPPORTED);
    f.platform.release(&f,NS_RADIO_ESP);
    NsWifiDiscovery d;ns_wifi_discovery_init(&d);
    assert(ns_wifi_discovery_begin(&d,0,5000,5000)==NS_UNSUPPORTED && !d.peer_may_scan);
    /* Raw identities must never reach the results renderer. */
    a.page=NS_PAGE_NETWORK_RESULTS;a.wifi_discovery.count=1;a.wifi_discovery.results_valid=true;
    strcpy(a.wifi_discovery.records[0].ap.ssid,"SYNTHETIC_PRIVATE");
    strcpy(a.wifi_discovery.records[0].ap.bssid,"02:00:00:00:00:01");
    a.wifi_discovery.records[0].ap.channel=6;a.wifi_discovery.records[0].ap.rssi=-60;
    NsFrame frame;ns_app_frame(&a,&frame);
    for(unsigned i=0;i<6;i++) assert(!strstr(frame.rows[i],"SYNTHETIC_PRIVATE") && !strstr(frame.rows[i],"02:00"));
    assert(strstr(frame.rows[4],"not live"));
    assert(ns_app_key(&a,NS_KEY_BACK,103)==NS_OK);
    assert(ns_app_key(&a,NS_KEY_BACK,104)==NS_OK && !a.running);
    ns_flipper_deinit(&f);assert(!mock_sdk_live() && !mock_sdk_records());
    puts("PASS: unified navigation, preserved tools, UART contention/cancellation, info-only gate and private results");
}
