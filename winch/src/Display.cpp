#include "Display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ---- Externally defined SPI Mutex ----
extern SemaphoreHandle_t spiMutex;

// Bold & Blue color palette - 16-bit RGB565 format
#define ACCENT_BLUE     0x1E9F  // Primary blue
#define DARK_BLUE       0x0010  // Darker blue for buttons
#define LIGHT_BLUE      0x867D  // Divider blue  
#define WARNING_ORANGE  0xFC60  // Armed state
#define ALERT_RED       0xF800  // Stopped state
#define PURE_WHITE      0xFFFF  // Text and temp

Display::Display() {}

void Display::init() {
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    M5.Lcd.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(BLACK);
    
    // Try to enable smoother text rendering if available
    M5.Lcd.setTextWrap(false);
    M5.Lcd.setTextDatum(TL_DATUM);  // Top-left alignment for consistent positioning
    
    drawStaticElements();
    xSemaphoreGive(spiMutex);
}

void Display::drawStaticElements() {
    // Main divider lines
    M5.Lcd.drawFastHLine(0, 50, 320, ACCENT_BLUE);     // Under STATE
    M5.Lcd.drawFastHLine(10, 90, 300, LIGHT_BLUE);     // Under line/current
    M5.Lcd.drawFastHLine(10, 130, 300, LIGHT_BLUE);    // Under status
    
    // Clean button labels - no frames
    drawButtonLabels();
}

void Display::drawButtonLabels() {
    M5.Lcd.setTextSize(2);  // Bigger buttons
    M5.Lcd.setTextColor(PURE_WHITE);
    
    // Clean button text only - bigger and better positioned
    M5.Lcd.setCursor(35, 210);
    M5.Lcd.print("TEST");
    M5.Lcd.setCursor(120, 210);
    M5.Lcd.print("SETUP");
    M5.Lcd.setCursor(245, 210);
    M5.Lcd.print("RESET");
}

void Display::updateDisplay(int currentState, int rssi, float snr, int lineOut,
                          int current, bool lineStopArmed, bool lineStopActivated,
                          int canTemperature, int scaledCurrent, int drumDiam) {
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    
    // Clear all data areas
    M5.Lcd.fillRect(0, 5, 320, 40, BLACK);      // STATE area
    M5.Lcd.fillRect(10, 55, 300, 30, BLACK);    // Line/Current area
    M5.Lcd.fillRect(10, 95, 300, 30, BLACK);    // Status area
    M5.Lcd.fillRect(10, 135, 300, 35, BLACK);   // Signal/RSSI/SNR area
    M5.Lcd.fillRect(10, 170, 300, 25, BLACK);   // Temperature area
    
    // STATE - HUGE and centered in pure white
    M5.Lcd.setTextSize(3);  // Slightly smaller but cleaner
    M5.Lcd.setTextColor(PURE_WHITE);  // Pure white for state
    
    // Center the state text
    String stateText = "STATE " + String(currentState);
    int textWidth = stateText.length() * 18; // Adjusted for size 3
    int centerX = (320 - textWidth) / 2;
    
    M5.Lcd.setCursor(centerX, 15);
    M5.Lcd.print(stateText);
    
    // LINE OUT and CURRENT - Bold blue, side by side
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(ACCENT_BLUE);
    
    M5.Lcd.setCursor(20, 60);
    M5.Lcd.printf("%dm", lineOut);
    
    M5.Lcd.setCursor(160, 60);
    M5.Lcd.printf("%dA D%d", scaledCurrent, drumDiam);
    
    // LINE STATUS - Centered and prominent, bigger size
    M5.Lcd.setTextSize(3);  // Bigger status text
    uint16_t statusColor;
    String statusText;
    
    if (lineStopActivated) {
        statusColor = ALERT_RED;
        statusText = "STOPPED";
    } else if (lineStopArmed) {
        statusColor = WARNING_ORANGE;
        statusText = "ARMED";
    } else {
        statusColor = ACCENT_BLUE;
        statusText = "RESET";
    }
    
    M5.Lcd.setTextColor(statusColor);
    
    // Center the status text better between the lines
    int statusWidth = statusText.length() * 18; // Adjusted for size 3
    int statusCenterX = (320 - statusWidth) / 2;
    
    M5.Lcd.setCursor(statusCenterX, 97);  // Moved slightly up for better centering
    M5.Lcd.print(statusText);
    
    // RSSI and SNR - Bold and important, with signal bars in center
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(ACCENT_BLUE);
    
    M5.Lcd.setCursor(20, 140);
    M5.Lcd.printf("%ddBm", rssi);
    
    M5.Lcd.setCursor(220, 140);
    M5.Lcd.printf("%.1fdB", snr);
    
    // Signal bars in center
    drawSignalBars(rssi);
    
    // TEMPERATURE - Centered and bigger, moved down slightly
    M5.Lcd.setTextSize(2);  // Bigger temp text
    uint16_t tempColor = PURE_WHITE;
    if (canTemperature > 70) tempColor = ALERT_RED;
    else if (canTemperature > 50) tempColor = WARNING_ORANGE;
    
    M5.Lcd.setTextColor(tempColor);
    String tempText = "Temp: " + String(canTemperature) + "C";
    int tempWidth = tempText.length() * 12; // Adjusted for size 2
    int tempCenterX = (320 - tempWidth) / 2;
    
    M5.Lcd.setCursor(tempCenterX, 170);  // Moved down slightly from 165
    M5.Lcd.print(tempText);
    
    xSemaphoreGive(spiMutex);
}

void Display::drawSignalBars(int rssi) {
    // Signal strength bars - centered between RSSI and SNR
    int signalStrength = map(constrain(rssi, -120, -30), -120, -30, 0, 5);
    
    // Center the 5 bars
    int startX = 140; // Center area
    
    for (int i = 0; i < 5; i++) {
        int barX = startX + i * 10;
        int barHeight = 6 + i * 2;
        uint16_t barColor = (i < signalStrength) ? ACCENT_BLUE : LIGHT_BLUE;
        
        M5.Lcd.fillRect(barX, 155 - barHeight, 6, barHeight, barColor);
    }
}

// Keep original function for compatibility
void Display::drawStaticLabels() {
    drawButtonLabels();
}

void Display::updateSettingsDisplay(int selectedState, int baseCurrents[7]) {
    xSemaphoreTake(spiMutex, portMAX_DELAY);

    M5.Lcd.fillRect(0, 0, 320, 200, BLACK);

    // Title
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(PURE_WHITE);
    M5.Lcd.setCursor(90, 5);
    M5.Lcd.print("AMP SETTINGS");

    M5.Lcd.drawFastHLine(0, 25, 320, ACCENT_BLUE);

    // List states 1-6 with their amp values
    for (int i = 1; i <= 6; i++) {
        int y = 30 + (i - 1) * 27;
        if (i == selectedState) {
            M5.Lcd.fillRect(5, y - 2, 310, 24, ACCENT_BLUE);
            M5.Lcd.setTextColor(BLACK);
        } else {
            M5.Lcd.setTextColor(PURE_WHITE);
        }
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(15, y);
        M5.Lcd.printf("State %d:  %3d A", i, baseCurrents[i]);
    }

    // Instructions
    M5.Lcd.drawFastHLine(0, 197, 320, ACCENT_BLUE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(LIGHT_BLUE);
    M5.Lcd.setCursor(5, 202);
    M5.Lcd.print("A:Select   C:+10A  LongC:-10A   B:Save+Exit");

    xSemaphoreGive(spiMutex);
}