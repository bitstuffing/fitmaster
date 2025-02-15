#include "connect.h"

PersistentState ps;
NetworkPacket np;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
bool isMaster = false;
bool masterDetected = false;
int scanStatus = WL_NO_SSID_AVAIL; // init
unsigned long scanStartTime = 0;
NetworkPacket slaveList[MAX_SLAVES];

unsigned long lastBeaconTime = 0;
const int BEACON_INTERVAL = 3000;
uint8_t slaveCount = 0;
unsigned long lastMasterBeacon = 0;

AsyncWebServer server(80);

unsigned long lastMaintenance = 0;
uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void generateUUID(char* uuid) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    uint8_t mac[6];
    WiFi.softAPmacAddress(mac);
    
    for(int i = 0; i < GROUP_UUID_SIZE - 1; i++) {
        uint8_t base = mac[i % 6] ^ (i * 37);
        uuid[i] = charset[base % 36];
    }
    uuid[GROUP_UUID_SIZE - 1] = '\0';
}

void forceMasterRole() {
    if(!isMaster) {
        WiFi.softAPdisconnect(true);
        becomeMaster();
    }
}

void sendNetworkBeacon() {
    if(millis() - lastBeaconTime > BEACON_INTERVAL) {
        static NetworkPacket beacon; 
        memcpy(beacon.uuid, ps.uuid, GROUP_UUID_SIZE);
        beacon.role = 1;
        beacon.last_seen = millis();
        WiFi.softAPmacAddress(beacon.mac);
        
        esp_err_t result = esp_now_send(broadcastMac, (uint8_t*)&beacon, sizeof(beacon));
        if(result != ESP_OK) {
            log_e("Error enviando beacon: %d", result);
        }
        lastBeaconTime = millis();
    }
}

void checkMasterPresence() {
    if(isMaster) return;
    
    if(millis() - lastMasterBeacon > MASTER_TIMEOUT * 2) {
        Serial.println("Master perdido - iniciando elección");
        startMasterElection();
    }
}

void resetInactivityTimer() {
    lastMaintenance = millis();
}

void startMasterElection() {
    masterDetected = false;
    unsigned long start = millis();
    
    while(millis() - start < MASTER_TIMEOUT) {
        if(checkForMaster()) {
            joinAsSlave();
            return;
        }
        delay(100);
    }
    
    becomeMaster();
    setupWebUI();
}

bool checkForMaster() {
    int n = WiFi.scanNetworks(true, true, 100, NULL, true);
    for(int i=0; i<n; i++){
        if(WiFi.SSID(i).startsWith("FitMaster-")) {
            WiFi.scanDelete();
            return true;
        }
    }
    WiFi.scanDelete();
    
    return masterDetected;
}

void becomeMaster() {
    isMaster = true;
    generateUUID(ps.uuid);
    ps.is_master = 1;
    EEPROM.put(0, ps);
    EEPROM.commit();

    WiFi.softAP(("FitMaster-" + String(ps.uuid)).c_str());
    esp_now_peer_info_t peer;
    memcpy(peer.peer_addr, broadcastMac, 6);
    esp_now_add_peer(&peer);
}

void joinAsSlave() {
    isMaster = false;
    ps.is_master = 0;
    EEPROM.put(0, ps);
    EEPROM.commit();
    
    WiFi.softAPdisconnect(true);
    Serial.println("Slave mode active");
}

void generateScanJSON(AsyncResponseStream *response) {
    int networkCount = -1;
    do{
        networkCount = WiFi.scanComplete();
    }while (networkCount < 0);

    response->print("[");
    bool first = true;
    for (int i = 0; i < min(networkCount, MAX_SCAN_RESULTS); i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.startsWith("FitMaster-")) {
            if (!first) response->print(",");
            response->printf("\"%s\"", ssid.substring(9).c_str());
            // response->printf("\"%s\"",ssid.c_str());
            first = false;
        }
    }
    response->print("]");
}


void startWiFiScan() {
  if(WiFi.scanComplete() != WIFI_SCAN_FAILED) return;
  
  WiFi.enableSTA(true);
  delay(50); 
  
  scanStartTime = millis();
  scanStatus = WiFi.scanNetworks(true, true, true, 100);
}

void setupWebUI() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<h1>Control</h1>"
                    "<button onclick='location.href=\"/master\"'>Master</button>"
                    "<button onclick='location.href=\"/scan\"'>Scan</button>";
        request->send(200, "text/html", html);
    });

    server.on("/master", HTTP_GET, [](AsyncWebServerRequest *request){
        forceMasterRole();
        request->redirect("/");
    });


    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        if (scanStatus != WIFI_SCAN_RUNNING){
            startWiFiScan();
        }else if (scanStatus == WIFI_SCAN_RUNNING && (millis() - scanStartTime < 15000)) {
            request->send(202, "application/json", "{\"status\":\"scanning\"}");
            return;
        }
        
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        generateScanJSON(response); // handler
        request->send(response);
        
        scanStatus = WIFI_SCAN_FAILED;
        WiFi.scanDelete();
    });

    server.on("/slaves", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "[";
        for(auto& slave : slaveList) {
            json += "{\"uuid\":\"" + String(slave.uuid) + "\",";
            json += "\"last_seen\":" + String(millis() - slave.last_seen) + "},";
        }
        if(json.endsWith(",")) json.remove(json.length()-1);
        json += "]";
        request->send(200, "application/json", json);
    });

    server.on("/remove", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("uuid")) {
            String uuidToRemove = request->getParam("uuid")->value();
            for(int i = 0; i < slaveCount; i++) {
                if(String(slaveList[i].uuid) == uuidToRemove) {
                    NetworkPacket command;
                    command.role = 2; // master
                    esp_now_send(slaveList[i].mac, (uint8_t*)&command, sizeof(command));
                    
                    // Shift the remaining slaves
                    for (int j = i; j < slaveCount - 1; j++) {
                        slaveList[j] = slaveList[j + 1];
                    }
                    slaveCount--;
                    break;
                }
            }
        }
        request->redirect("/");
    });

    server.begin();
}

void performMaintenance() {
    if(millis() - lastMaintenance > MAINTENANCE_INTERVAL) {
        // digitalWrite(BUZZER_PIN, HIGH);
        pixels.fill(pixels.Color(255, 255, 255));
        pixels.show();
        // delay(PULSE_DURATION);
        
        // digitalWrite(BUZZER_PIN, LOW);
        pixels.fill(pixels.Color(0, 0, 0));
        pixels.show();
        
        lastMaintenance = millis();
    }
}