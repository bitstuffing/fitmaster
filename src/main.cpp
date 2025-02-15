#include <Arduino.h>
#include "play.h"
#include "connect.h"

void setup() {
    EEPROM.begin(sizeof(PersistentState));
    EEPROM.get(0, ps);
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPdisconnect(true);
    
    if(esp_now_init() != ESP_OK) {
        ESP.restart();
    }

    esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
        if(len == sizeof(NetworkPacket)) {
            memcpy(&np, data, len);
            if(np.role == 1) resetInactivityTimer();
        }
    });

    pixels.begin();
    pinMode(BUZZER_PIN, OUTPUT);
    startMasterElection();
}

void loop(){
    playLoop();
    delay(1); 
}

void loopMasterSetup() {
    if(isMaster) {
        sendNetworkBeacon();
    } else {
        checkMasterPresence();
    }
    
    performMaintenance();
    
    if(millis() % 10000 == 0) {
        masterDetected = false;
    }

    if (scanStatus == WIFI_SCAN_RUNNING) {
        int result = WiFi.scanComplete();
        if (result >= 0 || (millis() - scanStartTime > 15000)) {
            scanStatus = result;
        }
    }
    
    delay(10); 
}
