#include "m5button.h"

#ifdef M5STICK
#ifdef PLUS
#include <M5StickCPlus.h>
#else
#include <M5StickC.h>
#endif
#endif

#if M5STICK
    static bool m5aButton;
    static bool m5bButton;
    static bool m5cButton;
    static bool m5displayDirty;
    static int m5displayCycles;
#endif

bool m5button::buttonA() {
    #ifdef M5STICK
        return m5aButton;
    #endif
    return false;
}

void m5button::Setup() {
    #ifdef M5STICK
        m5aButton = false;
        m5bButton = false;
        m5cButton = false;
        m5displayDirty = false;
    #endif
}

void m5button::Loop(){
    #if M5STICK
        M5.update();  // Read the press state of the key: A, B, C
        if (M5.BtnA.wasReleased()) {
            if (!m5aButton) {
                Serial.println("M5: button pressed");
                m5aButton = true;
                m5displayDirty=true;
            } else {
                Serial.println("M5: endOf: button pressed");
                M5.Lcd.fillScreen(BLACK);  // Set BLACK to the background color.
                M5.Lcd.setCursor(0, 0);
                M5.Axp.ScreenBreath(7);
                m5displayDirty=false;
                m5aButton=false;
            }
        } 
    #endif
}