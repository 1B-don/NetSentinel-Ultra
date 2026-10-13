#include "../ap_parser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(const char* input, const char* ssid, const char* bssid, int rssi, int channel) {
    NsAccessPoint ap;
    assert(ns_parse_ap_line(input, &ap));
    assert(strcmp(ap.ssid, ssid) == 0);
    assert(strcmp(ap.bssid, bssid) == 0);
    assert(ap.rssi == rssi);
    assert(ap.channel == channel);
}

int main(void) {
    check("> RSSI: -57 Ch: 3 BSSID: 50:ff:20:84:d6:0f ESSID: Octoglass",
          "Octoglass", "50:ff:20:84:d6:0f", -57, 3);
    check("-59 Ch: 3 00:11:22:33:44:55 ESSID: example",
          "example", "00:11:22:33:44:55", -59, 3);
    check("RSSI: -83 CH: 11 BSSID: AA:bb:C0:11:22:33 ESSID: ",
          "<hidden>", "AA:bb:C0:11:22:33", -83, 11);
    check("[0][CH:3] Octoglass -66", "Octoglass", "", -66, 3);
    check("[12][CH:11] Home Network 2.4 -90", "Home Network 2.4", "", -90, 11);
    NsAccessPoint ap;
    assert(!ns_parse_ap_line("Starting AP scan. Stop with stopscan", &ap));
    assert(!ns_parse_ap_line("Beacon: 11 18 1 17464", &ap));
    assert(!ns_parse_ap_line("RSSI: -44 Ch: 36 BSSID: 00:11:22:33:44:55 ESSID: 5G", &ap));
    assert(!ns_parse_ap_line("RSSI: -44 Ch: 3 BSSID: not-a-mac ESSID: test", &ap));
    assert(!ns_parse_ap_line("[0][CH:3] no rssi", &ap));
    assert(!ns_parse_ap_line("[x][CH:3] bad -50", &ap));
    assert(!ns_parse_ap_line("[0][CH:3x] bad -50", &ap));
    assert(!ns_parse_ap_line("[0][CH:3] bad -50x", &ap));
    assert(!ns_parse_ap_line("[0][CH:3] bad -9999999999999999999999999", &ap));
    assert(!ns_parse_ap_line(NULL, &ap));
    puts("PASS: 15 Marauder parser scenarios");
}
