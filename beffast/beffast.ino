#if !defined(ARDUINO_ESP32_DEV) && !defined(ARDUINO_INKPLATE6V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate6 or Soldered Inkplate6 in the boards menu."
#endif

#include "Inkplate.h"
#include <WiFi.h>

Inkplate display(INKPLATE_3BIT);
WiFiServer server(80);

#include "./wificreds.h"

const char connecting[] = "Connecting to Wifi~";
const char lily_hoongy_beffast[] = "Lily has not had BEFFAST yet :'(";
const char lily_chomped_beffast[] = "Lily has chomped BEFFAST :)";
const char lily_hoongy_dindin[] = "Lily has not had DINDIN yet :'(";
const char lily_chomped_dindin[] = "Lily has chomped DINDIN :)";

static void textCentered(char const *text) {
    int16_t x, y;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x, &y, &w, &h);
    display.setCursor((display.width() - w) / 2, (display.height() - h) / 2);
    display.print(text);
}

void setup()
{
    display.begin();
    display.setTextColor(0, 7);
    display.setTextSize(4);
    display.setTextWrap(true);

    display.clearDisplay();
    textCentered(connecting);
    display.display();

    WiFi.begin(wifi_ssid, wifi_pass);

    while (true) {
        int status = WiFi.status();
        if (status == WL_CONNECTED) {
            break;
        } else if (status == WL_CONNECT_FAILED) {
            display.clearDisplay();
            display.display();
            while (true) {
                delay(60000);
            }
        }
        delay(500);
    };

    display.clearDisplay();
    textCentered(lily_hoongy_beffast);
    display.display();

    server.begin();
}

void loop()
{
    WiFiClient client = server.available();
    if (client) {

    }
}
