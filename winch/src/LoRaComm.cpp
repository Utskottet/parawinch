#include "LoRaComm.h"
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ---- Externally defined SPI Mutex ----
extern SemaphoreHandle_t spiMutex;

// ---- SX1262 Pin Map (adjust for your wiring if needed) ----
#define NSS   5
#define DIO1  34
#define RST   13
#define BUSY  26
#define SCK   18
#define MISO  19
#define MOSI  23

// ---- Interrupt flag and handler ----
static volatile bool rxFlag = false;
static void IRAM_ATTR setFlag() {
    rxFlag = true;
}

// ---- LoRaComm implementation ----

void LoRaComm::init() {
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SPI.begin(SCK, MISO, MOSI);
    lora = new SX1262(new Module(NSS, DIO1, RST, BUSY));
    int e = lora->begin(LORA_FREQUENCY_MHZ);
    if (e) {
        Serial.printf("LoRa init error %d\n", e);
        while (true);
    }
    lora->setBandwidth(LORA_BANDWIDTH_KHZ);
    lora->setSpreadingFactor(LORA_SPREADING_FACTOR);
    lora->setCodingRate(LORA_CODING_RATE);
    lora->setOutputPower(LORA_OUTPUT_POWER_DBM);
    // lora->setCurrentLimit(LORA_CURRENT_LIMIT_MA);  // temporarily disabled to isolate RX desense
    lora->setDio1Action(setFlag);        // Attach interrupt handler
    lora->startReceive();                // Ready to receive packets
    xSemaphoreGive(spiMutex);
}

bool LoRaComm::receiveCmd(CmdPacket& outCmd) {
    if (!rxFlag) return false;
    rxFlag = false;
    uint8_t buf[16];

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    size_t pktLen = lora->getPacketLength();
    int result = lora->readData(buf, sizeof(buf));
    xSemaphoreGive(spiMutex);

    if (result == RADIOLIB_ERR_NONE && pktLen >= sizeof(CmdPacket)) {
        if (buf[0] == 0x01) { // CmdPacket type
            outCmd = *(CmdPacket*)buf;
            xSemaphoreTake(spiMutex, portMAX_DELAY);
            lora->startReceive();
            xSemaphoreGive(spiMutex);
            return true;
        }
        if (buf[0] == 0x05 && pktLen >= sizeof(ConfigPacket)) {
            _lastConfigPacket = *(ConfigPacket*)buf;
            _configPending = true;
            Serial.printf("[LoRa] Config packet received, len=%d\n", pktLen);
        }
    }
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    lora->startReceive();
    xSemaphoreGive(spiMutex);
    return false;
}

bool LoRaComm::hasConfigPacket(ConfigPacket& out) {
    if (!_configPending) return false;
    out = _lastConfigPacket;
    _configPending = false;
    return true;
}

void LoRaComm::sendMetrics(const MetricsPacket& m) {
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    lora->transmit((uint8_t*)&m, sizeof(m));
    lora->finishTransmit();
    lora->startReceive();
    xSemaphoreGive(spiMutex);
}

void LoRaComm::sendAck(const AckPacket& ack) {
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    lora->transmit((uint8_t*)&ack, sizeof(ack));
    lora->finishTransmit();
    lora->startReceive();
    xSemaphoreGive(spiMutex);
}

int LoRaComm::getLastRSSI() {
    if (lora) return lora->getRSSI();
    return 0;
}
float LoRaComm::getLastSNR() {
    if (lora) return lora->getSNR();
    return 0.0;
}
