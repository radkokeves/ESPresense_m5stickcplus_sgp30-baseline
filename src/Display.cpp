#include "Display.h"
#include "m5button.h"

#ifdef M5STICK
//#include "tb_display.h"
#ifdef PLUS
#include <M5StickCPlus.h>
#else
#include <M5StickC.h>
#endif
#endif

void Display::Setup() {
}

void Display::Status(const char *format, ...) {
    char *message;
    va_list args;
    va_start(args, format);
    vasprintf(&message, format, args);
    va_end(args);

    if (m5button::buttonA()) {
        M5.Axp.ScreenBreath(10);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(WHITE, BLACK);
        //tb_display_print_String(message);
        M5.Lcd.print(message);        
    }
    free(message);
}