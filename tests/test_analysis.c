#include "../ns_analysis.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    NsAccessPoint a[3] = {
        {.ssid="alpha", .bssid="00:11:22:33:44:55", .channel=1, .rssi=-60},
        {.ssid="beta", .bssid="00:11:22:33:44:66", .channel=6, .rssi=-43},
        {.ssid="gamma", .channel=11, .rssi=-78},
    };
    NsAccessPoint b[3] = {
        {.ssid="alpha", .bssid="00:11:22:33:44:55", .channel=11, .rssi=-60},
        {.ssid="new", .bssid="00:11:22:33:44:77", .channel=6, .rssi=-70},
        {.ssid="gamma", .channel=11, .rssi=-76},
    };
    uint8_t ch[14]; ns_channel_counts(a, 3, ch);
    assert(ch[0] == 1 && ch[5] == 1 && ch[10] == 1 && ch[1] == 0);
    NsDiff diff = ns_diff(a, 3, b, 3);
    assert(diff.new_aps == 1 && diff.missing_aps == 1 && diff.channel_changes == 1);
    char line[128];
    assert(ns_csv_ap(&a[0], line, sizeof(line)));
    assert(strcmp(line, "\"redacted\",\"\",1,-60\n") == 0);
    strcpy(a[0].ssid, "a,\"z");
    assert(ns_csv_ap(&a[0], line, sizeof(line)));
    assert(!strstr(line,"a,") && !strstr(line,"00:11"));
    assert(!ns_csv_ap(&a[0], line, 7));
    puts("Analysis tests passed");
    return 0;
}
