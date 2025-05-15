#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ArduinoJson.h>
#include "M5Dial.h"

class WifiManager {
public:
    typedef enum {
        WIFI_CONNECT_SUCCESS = 0,
        WIFI_CONNECT_ERROR_JSON = 1,
        WIFI_CONNECT_ERROR_NO_NETWORKS = 2,
        WIFI_CONNECT_ERROR_FAILED = 3
    } wifi_connect_status_t;

    WifiManager();
    wifi_connect_status_t connect(const String& settingsJson);
    void disconnect();
    void updateDisplay() const;
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

private:
    static const uint8_t MAX_RETRIES = 5U;
    static const uint16_t DELAY_BETWEEN_RETRIES_MS = 5000U;
    
    String lastSuccessfulSSID;
    String lastSuccessfulPassword;

    bool tryConnection(const char* ssid, const char* password, uint8_t numRetries);
    void displayStatus(const char* ssid = nullptr) const;
};

#endif // WIFI_MANAGER_H