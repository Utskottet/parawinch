// Display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Stack.h>

class Display {
public:
    Display();
    void init();
    void drawStaticLabels();
    void updateDisplay(int currentState, int rssi, float snr, int lineOut, int current, bool lineStopArmed, bool lineStopActivated, int canTemperature, int scaledCurrent, int drumDiam);
    void updateSettingsDisplay(int selectedState, int baseCurrents[7]);

private:
    void drawStaticElements();
    void drawButtonLabels();
    void drawSignalBars(int rssi);
};

#endif