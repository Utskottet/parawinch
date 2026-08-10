#include <M5Stack.h>
#include <Preferences.h>
#include "LoRaComm.h"
#include "Lorastruct.h"
#include "Display.h"
#include "StepperControl.h"
#include "can.h"

// ---- FreeRTOS SPI Mutex ----
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
SemaphoreHandle_t spiMutex; // Global mutex instance

// ---- Globals/State ----
LoRaComm lora;
Display display;
StepperControl stepper(lora);
Preferences prefs;

volatile int lastLoRaRSSI = 0;
volatile float lastLoRaSNR = 0.0;

extern int32_t can_distance;
extern int32_t can_current;
extern int32_t can_temperature;

bool buttonToggled = false;
int32_t lineOutOffset = 0;

bool armLineStop = false;
bool lineStopActivated = false;
const int armLineStopThreshold = 40;
const int lineStopThreshold = 35;

unsigned long lastButtonPressTime = 0;
int buttonCPressCount = 0;

int currentState = 0;

// ---- Amp table (state 0-6, amps per state) ----
int baseCurrents[7] = {0, 20, 40, 60, 100, 120, 140};

// ---- Settings mode ----
bool settingsMode = false;
int settingsSelectedState = 1;
bool forceDisplayRedraw = false;

// ---- Drum compensation ----
const float DRUM_CORE_DIAM = 150.0f;
const float DRUM_FULL_DIAM = 265.0f;
const float TOTAL_LINE_M   = 1500.0f;
volatile int16_t scaledCurrent = 0;
volatile float estimatedDrumDiam = 265.0f;

float estimateDrumDiamMm(int32_t lineOutMeters) {
    float lineOut = constrain((float)lineOutMeters, 0.0f, TOTAL_LINE_M);
    float lineOnDrum = TOTAL_LINE_M - lineOut;
    float fraction = lineOnDrum / TOTAL_LINE_M;
    float rCore = DRUM_CORE_DIAM / 2.0f;
    float rFull = DRUM_FULL_DIAM / 2.0f;
    float radius = sqrtf(rCore * rCore + fraction * (rFull * rFull - rCore * rCore));
    return radius * 2.0f;
}

static unsigned long lastDisplay = 0;  // display only

// ------------ LoRa FreeRTOS Task -------------
void LoRaTask(void *pvParameters) {
    static uint8_t lastCmdSeq = 0;
    static uint8_t metricsSeq = 0;
    static unsigned long lastMetrics = 0;

    for (;;) {
        CmdPacket cmd;

        // 1. Handle incoming commands
        if (lora.receiveCmd(cmd)) {
            currentState = cmd.state;
            lastCmdSeq = cmd.seq;

            // Only allow state change from remote if NOT lineStopActivated
            if (!lineStopActivated) {
                currentState = cmd.state;
            } else {
                Serial.println("[LoRaTask] LineStop active - ignoring remote state change!");
            }

            // Send ACK
            AckPacket ack;
            ack.type = 0x03;
            ack.seq  = cmd.seq;
            ack.pad0 = 0;
            ack.pad1 = 0;
            lora.sendAck(ack);

            // Send metrics immediately after command
            MetricsPacket m;
            m.type        = 0x02;
            m.seq         = metricsSeq++;
            m.lastCmdSeq  = lastCmdSeq;
            m.amps_x10    = can_current * 10;
            m.distance_m  = max((int32_t)0, -(can_distance - lineOutOffset));
            m.vesc_mV     = 3700; // Replace with real value if available

            // --- State mapping for remote ---
            if (lineStopActivated) {
                m.lineState = 'S';    // Stopped
            } else if (armLineStop) {
                m.lineState = 'A';    // Armed
            } else {
                m.lineState = 'R';    // Ready
            }
            m.scaled_amps_x10 = scaledCurrent * 10;

            // Debug: print the outgoing packet
            uint8_t* p = (uint8_t*)&m;
            Serial.print("[LoRaTask] Raw metrics bytes (after cmd): ");
            for (int i = 0; i < sizeof(MetricsPacket); ++i) {
                Serial.printf("%02X ", p[i]);
            }
            Serial.println();

            lora.sendMetrics(m);
            lastMetrics = millis();

            // Update RSSI/SNR for local display
            lastLoRaRSSI = lora.getLastRSSI();
            lastLoRaSNR  = lora.getLastSNR();
        }

        // 2. Send periodic metrics
        if (millis() - lastMetrics > METRICS_PERIOD_MS) {
            MetricsPacket m;
            m.type        = 0x02;
            m.seq         = metricsSeq++;
            m.lastCmdSeq  = lastCmdSeq;
            m.amps_x10    = can_current * 10;
            m.distance_m  = max((int32_t)0, -(can_distance - lineOutOffset));
            m.vesc_mV     = 3700;

            if (lineStopActivated) {
                m.lineState = 'S';
            } else if (armLineStop) {
                m.lineState = 'A';
            } else {
                m.lineState = 'R';
            }
            m.scaled_amps_x10 = scaledCurrent * 10;

            uint8_t* p = (uint8_t*)&m;
            Serial.print("[LoRaTask] Raw metrics bytes (periodic): ");
            for (int i = 0; i < sizeof(MetricsPacket); ++i) {
                Serial.printf("%02X ", p[i]);
            }
            Serial.println();

            lora.sendMetrics(m);
            lastMetrics = millis();

            // Update RSSI/SNR for local display
            lastLoRaRSSI = lora.getLastRSSI();
            lastLoRaSNR  = lora.getLastSNR();
        }

        vTaskDelay(1); // Always yield to other tasks
    }
}




// ------------ Setup & Main Loop --------------

void setup() {
    Serial.begin(115200);
    Serial.println("Starting setup");

    spiMutex = xSemaphoreCreateMutex();   // CREATE MUTEX FIRST
    Serial.println("Mutex created");

    // Load amp values from NVS (fall back to defaults if not set)
    prefs.begin("winch", true); // read-only
    baseCurrents[1] = prefs.getInt("amp1", 20);
    baseCurrents[2] = prefs.getInt("amp2", 40);
    baseCurrents[3] = prefs.getInt("amp3", 60);
    baseCurrents[4] = prefs.getInt("amp4", 100);
    baseCurrents[5] = prefs.getInt("amp5", 120);
    baseCurrents[6] = prefs.getInt("amp6", 140);
    prefs.end();
    Serial.println("NVS loaded");

    M5.begin();
    Serial.println("M5Stack initialized");

    lora.init();
    Serial.println("LoRa initialized");

    // ---- Start LoRa task on core 1 with high priority ----
    xTaskCreatePinnedToCore(
        LoRaTask,        // Task function
        "LoRaTask",      // Name
        4096,            // Stack size
        NULL,            // Param
        2,               // Priority (2=high, 1=normal, 0=idle)
        NULL,            // Task handle (not used)
        1                // Core 1 (best for ESP32, keeps WiFi/BT on core 0)
    );
    Serial.println("LoRaTask started");

    display.init();
    Serial.println("Display initialized");

    stepper.setup();
    Serial.println("Stepper initialized");

    can_setup();
    Serial.println("CAN initialized");
}

void loop() {
    // --- Button B: Toggle settings mode ---
    if (M5.BtnB.wasPressed()) {
        if (settingsMode) {
            // Exit settings: save all amp values to NVS
            prefs.begin("winch", false);
            prefs.putInt("amp1", baseCurrents[1]);
            prefs.putInt("amp2", baseCurrents[2]);
            prefs.putInt("amp3", baseCurrents[3]);
            prefs.putInt("amp4", baseCurrents[4]);
            prefs.putInt("amp5", baseCurrents[5]);
            prefs.putInt("amp6", baseCurrents[6]);
            prefs.end();
            settingsMode = false;
            forceDisplayRedraw = true;
            display.init(); // restore static UI elements
            Serial.println("Settings saved, exiting settings mode");
        } else {
            settingsMode = true;
            settingsSelectedState = 1;
            display.updateSettingsDisplay(settingsSelectedState, baseCurrents);
            Serial.println("Entering settings mode");
        }
    }

    // --- Button C: line reset / deactivate (normal mode only) ---
    if (!settingsMode && M5.BtnC.wasPressed()) {
        unsigned long currentTime = millis();
        if (currentTime - lastButtonPressTime < 500) {
            buttonCPressCount++;
        } else {
            buttonCPressCount = 1;
        }
        lastButtonPressTime = currentTime;

        if (buttonCPressCount == 2) {
            lineStopActivated = false;
            armLineStop = false;
            Serial.println("LineStop deactivated");
        } else {
            lineOutOffset = can_distance;
            Serial.println("LineOut reset");
        }
    }

    // --- Calculate adjusted LineOut value ---
    int32_t adjustedLineOut = max((int32_t)0, -(can_distance - lineOutOffset));

    // --- Arm/Trigger line stop (always runs) ---
    if (!armLineStop && adjustedLineOut > armLineStopThreshold) {
        armLineStop = true;
        Serial.println("LineStop armed");
    }
    if (armLineStop && !lineStopActivated && adjustedLineOut < lineStopThreshold) {
        lineStopActivated = true;
        Serial.println("LineStop activated");
    }

    // --- Failsafe: Force STOP if lineStopActivated ---
    if (lineStopActivated) {
        currentState = 0;
    }

    // --- Drum compensation: scale current by drum diameter ---
    estimatedDrumDiam = estimateDrumDiamMm(adjustedLineOut);
    float scaleFactor = estimatedDrumDiam / DRUM_FULL_DIAM;
    scaledCurrent = constrain((int16_t)(baseCurrents[currentState] * scaleFactor), 0, 200);
    Serial.printf("[Drum] rawCAN=%d offset=%d lineOut=%d drum=%.0fmm scale=%.2f base=%d scaled=%d\n",
                  can_distance, lineOutOffset, adjustedLineOut, estimatedDrumDiam, scaleFactor,
                  baseCurrents[currentState], scaledCurrent);

    // --- Settings mode: navigation and display ---
    if (settingsMode) {
        // Button A: cycle selected state 1→2→3→4→5→6→1
        if (M5.BtnA.wasPressed()) {
            settingsSelectedState = (settingsSelectedState % 6) + 1;
            display.updateSettingsDisplay(settingsSelectedState, baseCurrents);
        }

        // Button C short press: +10A, long press: -10A
        static unsigned long btnCSettingsStart = 0;
        if (M5.BtnC.wasPressed()) {
            btnCSettingsStart = millis();
        }
        if (M5.BtnC.wasReleased()) {
            if (millis() - btnCSettingsStart > 500) {
                // Long press: decrement
                baseCurrents[settingsSelectedState] -= 10;
                if (baseCurrents[settingsSelectedState] < 0) baseCurrents[settingsSelectedState] = 200;
            } else {
                // Short press: increment
                baseCurrents[settingsSelectedState] += 10;
                if (baseCurrents[settingsSelectedState] > 200) baseCurrents[settingsSelectedState] = 0;
            }
            display.updateSettingsDisplay(settingsSelectedState, baseCurrents);
        }

    } else {
        // --- Normal display dirty-check ---
        static int lastDisplayedState = -1;
        static int lastDisplayedLineOut = -1;
        static int lastDisplayedCurrent = -9999;
        static bool lastDisplayedLineStopArmed = false;
        static bool lastDisplayedLineStopActivated = false;
        static int lastDisplayedTemperature = -9999;
        static int lastDisplayedRSSI = 12345;    // impossible value to force update first loop
        static float lastDisplayedSNR = 12345.0; // impossible value to force update first loop
        static int lastDisplayedScaledCurrent = -9999;
        static int lastDisplayedDrumDiam = -1;

        if (
            forceDisplayRedraw ||
            currentState != lastDisplayedState ||
            adjustedLineOut != lastDisplayedLineOut ||
            can_current != lastDisplayedCurrent ||
            armLineStop != lastDisplayedLineStopArmed ||
            lineStopActivated != lastDisplayedLineStopActivated ||
            can_temperature != lastDisplayedTemperature ||
            lastLoRaRSSI != lastDisplayedRSSI ||
            lastLoRaSNR != lastDisplayedSNR ||
            scaledCurrent != lastDisplayedScaledCurrent ||
            (int)estimatedDrumDiam != lastDisplayedDrumDiam
        ) {
            display.updateDisplay(
                currentState,
                lastLoRaRSSI,
                lastLoRaSNR,
                adjustedLineOut,
                can_current,
                armLineStop,
                lineStopActivated,
                can_temperature,
                scaledCurrent,
                (int)estimatedDrumDiam
            );
            lastDisplayedState = currentState;
            lastDisplayedLineOut = adjustedLineOut;
            lastDisplayedCurrent = can_current;
            lastDisplayedLineStopArmed = armLineStop;
            lastDisplayedLineStopActivated = lineStopActivated;
            lastDisplayedTemperature = can_temperature;
            lastDisplayedRSSI = lastLoRaRSSI;
            lastDisplayedSNR = lastLoRaSNR;
            lastDisplayedScaledCurrent = scaledCurrent;
            lastDisplayedDrumDiam = (int)estimatedDrumDiam;
            forceDisplayRedraw = false;
        }

        // --- Button A: Manual toggle stepper (normal mode only) ---
        if (M5.BtnA.wasPressed()) {
            buttonToggled = !buttonToggled;
        }
    }

    // --- Stepper logic (always runs) ---
    if (buttonToggled) {
        stepper.runStepper();
    } else {
        if (currentState == 0) {
            stepper.stopAndCenter();
        } else {
            stepper.runStepper();
        }
    }
    stepper.update();

    // --- CAN (always runs) ---
    can_send_message();
    can_receive_message();

    M5.update();
    delay(5); // keep loop responsive
}
