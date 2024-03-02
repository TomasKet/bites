#include <stdbool.h>

void wifi_sta_init(const char* ssid, const char* password);

struct wifi_sta {
    bool is_internet;
};

extern struct wifi_sta wifi_sta;