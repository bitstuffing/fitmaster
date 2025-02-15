#ifndef CONNECT_H
#define CONNECT_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "hardware.h"

// network configuration
#define MASTER_TIMEOUT 5000
#define GROUP_UUID_SIZE 16
#define MAINTENANCE_INTERVAL 15000

#define MAX_SLAVES 5
#define MAX_SCAN_RESULTS 10

typedef struct {
    char uuid[GROUP_UUID_SIZE];
    uint8_t role;
    uint32_t last_seen;
    uint8_t mac[6];
} NetworkPacket;

typedef struct {
    char uuid[GROUP_UUID_SIZE];
    uint8_t is_master;
} PersistentState;

extern PersistentState ps;
extern NetworkPacket np;
extern bool isMaster;
extern bool masterDetected;
extern int scanStatus;
extern unsigned long scanStartTime;
extern NetworkPacket slaveList[MAX_SLAVES];
extern Adafruit_NeoPixel pixels;

void startMasterElection();
void becomeMaster();
void joinAsSlave();
void performMaintenance();
void resetInactivityTimer();
bool checkForMaster();
void sendNetworkBeacon();
void checkMasterPresence();
void generateUUID(char* uuid);
void forceMasterRole();
void setupWebUI();
void generateScanJSON(AsyncResponseStream *response);
void startWiFiScan();

#endif // CONNECT_H