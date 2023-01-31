#include "Display.h"

#ifdef M5STICK
#ifdef PLUS
#include <M5StickCPlus.h>
#else
#include <M5StickC.h>
#endif
//#include <TFT_eSPI.h>
//#include "tb_display.h"
#endif

void Display::Setup() {
#ifdef M5STICK
#ifdef PLUS
    //tb_display_init();
#endif
#endif
}
void Display::Status(const char *format, ...) {
    char *message;
    va_list args;
    va_start(args, format);
    vasprintf(&message, format, args);
    va_end(args);
#ifdef M5STICK
#ifdef PLUS
    //tb_display_print_String(message);
/*
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Axp.ScreenBreath(0x0001);
    delay(500);
    M5.Lcd.printf(message);
*/
#endif
#endif
    free(message);
}
