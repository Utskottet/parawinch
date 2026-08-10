#include <Arduino.h>
#include <cstring>
#include "can.h"
#include "driver/twai.h"
#include "LoRaComm.h"

extern int currentState;
extern int baseCurrents[7];
extern volatile int16_t scaledCurrent;
extern LoRaComm lora;

const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_17, GPIO_NUM_16, TWAI_MODE_NORMAL);
const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

// Global variables to hold CAN data
int32_t can_distance = 0;
int32_t can_current = 0;
int32_t can_temperature = 0;

void can_setup() {
    Serial.println("Installing TWAI driver...");
    twai_driver_install(&g_config, &t_config, &f_config);
    Serial.println("Starting TWAI driver...");
    twai_start();
}

void can_send_state_sid(int state) {
    uint8_t buffer[4] = {0};
    buffer[0] = (uint8_t)((state >> 24) & 0xFF);
    buffer[1] = (uint8_t)((state >> 16) & 0xFF);
    buffer[2] = (uint8_t)((state >> 8) & 0xFF);
    buffer[3] = (uint8_t)(state & 0xFF);
    twai_message_t message;
    message.identifier = 10;
    message.extd = 0;  // Standard ID (SID) — matches Lisp can-send-sid
    message.rtr = 0;
    message.data_length_code = 4;
    memcpy(message.data, buffer, 4);
    twai_transmit(&message, pdMS_TO_TICKS(10));
}

void can_send_message() {
    static unsigned long lastSent = millis();
    if (millis() - lastSent >= 200) {
        send_custom_can_message(1, scaledCurrent);
        can_send_state_sid(currentState);
        lastSent = millis();
    }
}

void can_receive_message() {
    twai_message_t message;
    if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK) {
        if (message.data_length_code >= 4) {  // Make sure we have at least 4 bytes
            if (message.identifier == 7) {
                // Distance from ID 7
                can_distance = (int32_t)((message.data[0] << 24) | (message.data[1] << 16) | (message.data[2] << 8) | message.data[3]);
            }
            else if (message.identifier == 8) {
                // Current from ID 8
                can_current = (int32_t)((message.data[0] << 24) | (message.data[1] << 16) | (message.data[2] << 8) | message.data[3]);
            }
            else if (message.identifier == 9) {
                // Temperature from ID 9
                can_temperature = (int32_t)((message.data[0] << 24) | (message.data[1] << 16) | (message.data[2] << 8) | message.data[3]);
            }
        }
    }
}

void can_transmit_eid(uint32_t id, const uint8_t *data, uint8_t len) {
    twai_message_t message;
    message.identifier = id;
    message.extd = 1;  // Extended ID
    message.rtr = 0;   // Data frame
    message.data_length_code = len;
    memcpy(message.data, data, len);
    twai_transmit(&message, pdMS_TO_TICKS(1000));
}

void send_custom_can_message(uint8_t controller_id, int16_t amps) {
    uint8_t buffer[2];
    buffer[0] = (uint8_t)((amps >> 8) & 0xFF); // High byte
    buffer[1] = (uint8_t)(amps & 0xFF);         // Low byte
    uint32_t id = ((uint32_t)controller_id) | ((uint32_t)CAN_PACKET_SET_CURRENT << 8);
    can_transmit_eid(id, buffer, 2);
}