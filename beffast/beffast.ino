#if !defined(ARDUINO_ESP32_DEV) && !defined(ARDUINO_INKPLATE6V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate6 or Soldered Inkplate6 in the boards menu."
#endif

#include <Inkplate.h>
#include <WiFi.h>
#include <NTPClient.h>

Inkplate display(INKPLATE_3BIT);
WiFiServer server(7903);

#include "./wificreds.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char CONNECTING[] = "Connecting to WiFi~";
const char CONNECT_FAILED[] = "WiFi connect fail :<";
const char SYNCING_NTP[] = "Syncing with NTP!";
const char* DAYS_OF_WEEK[] = {
    "domingo",
    "lunes",
    "martes",
    "miercoles",
    "jueves",
    "viernes",
    "snabado",
};

const char* STATE_TEXTS[] = {
    "Lily has not had BEFFAST yet :'(",
    "Lily has chomped BEFFAST :)",
    "Lily has not had DINDIN yet :'(",
    "Lily has chomped DINDIN :)",
};

static enum {
    LILY_HOONGY_BEFFAST,
    LILY_CHOMPED_BEFFAST,
    LILY_HOONGY_DINDIN,
    LILY_CHOMPED_DINDIN,
} state_of_the_dog;

static tm timeinfo;

static void displayTextCentered(char const* text)
{
    static int16_t x, y;
    static uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x, &y, &w, &h);
    display.setCursor((display.width() - w) / 2, (display.height() - h) / 2);
    display.print(text);
}

static void displayTextRightLn(char const* text)
{
    static int16_t x, y;
    static uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x, &y, &w, &h);
    display.setCursor(display.width() - w, display.getCursorY());
    display.println(text);
}

static void displayClock()
{
    display.setCursor(0, 0);
    updateTimeinfo();
    displayTextRightLn(DAYS_OF_WEEK[timeinfo.tm_wday]);
    static char timebuf[10];
    static size_t len;
    len = strftime(timebuf, sizeof(timebuf), "%I:%M %p", &timeinfo);
    if (timebuf[0] == '0') {
        timebuf[0] = ' ';
    }
    timebuf[len - 2] = tolower(timebuf[len - 2]);
    timebuf[len - 1] = tolower(timebuf[len - 1]);
    displayTextRightLn(timebuf);
}

static void refresh()
{
    display.clearDisplay();
    displayClock();
    displayTextCentered(STATE_TEXTS[state_of_the_dog]);
    display.display();
}

void setup()
{
    // Australia/Melbourne
    setenv("TZ", "AEST-10AEDT,M10.1.0,M4.1.0/3", 1);
    tzset();

    state_of_the_dog = LILY_HOONGY_BEFFAST;

    display.begin();
    display.setTextColor(0, 7);
    display.setTextSize(4);
    display.setTextWrap(true);

    display.clearDisplay();
    displayTextCentered(CONNECTING);
    display.display();

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    bool attempted = false;

    while (true) {
        int status = WiFi.status();
        if (status == WL_CONNECTED) {
            break;
        } else if (!attempted) {
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            attempted = true;
        } else if (status == WL_CONNECT_FAILED) {
            display.clearDisplay();
            displayTextCentered(CONNECT_FAILED);
            display.display();
            while (true) {
                delay(60000);
            }
        }
        delay(500);
    };

    server.begin();
    timeClient.begin();

    display.clearDisplay();
    displayTextCentered(SYNCING_NTP);
    display.display();

    timeClient.update();
    while (!timeClient.isTimeSet()) {
        timeClient.update();
    }

    refresh();
}

enum struct RequestKind {
    FEED,
    UNFEED,
};

static struct {
    RequestKind kind;
} request;

#define URI_FEED "/feed"
#define URI_UNFEED "/unfeed"

static bool parseClientRequest(WiFiClient& client)
{
    static enum {
        VERB,     // "POST"
        URI,      // "/feed"
        HTTP,     // "HTTP/1.1"
        HEADERS,  // etc.
    } parse_state;
    static char buf[255];
    static uint8_t len;

    static unsigned long start;

    parse_state = VERB;
    len = 0;

    start = millis();

    while (client.connected()) {
        if (!client.available()) {
            if (millis() - start > 5000) {
                return false;
            }
            delay(10);
            continue;
        }
        char c = client.read();
        switch (parse_state) {
        case VERB:
            if (c >= 'A' && c <= 'Z' && len < 254) {
                buf[len++] = c;
            } else if (c == ' ') {
                buf[len] = 0;
                if (strcmp(buf, "POST") != 0) {
                    return false;
                }
                parse_state = URI;
                len = 0;
            } else {
                return false;
            }
            break;
        case URI:
            if ((c == '/'
                 || c == '_'
                 || (c >= 'a' && c <= 'z')
                 || (c >= '0' && c <= '9'))
                && len < 254) {
                buf[len++] = c;
            } else if (c == ' ') {
                buf[len] = 0;
                if (strcmp(buf, URI_FEED) == 0) {
                    request.kind = RequestKind::FEED;
                } else if (strcmp(buf, URI_UNFEED) == 0) {
                    request.kind = RequestKind::UNFEED;
                } else {
                    return false;
                }
                parse_state = HTTP;
                len = 0;
            } else {
                return false;
            }
            break;
        case HTTP:
            if ((c == '/'
                 || c == '1'
                 || c == '.'
                 || c == '0'
                 || (c >= 'A' && c <= 'Z'))
                && len < 254) {
                buf[len++] = c;
            } else if (c == '\r') {
                buf[len] = 0;
                if (strcmp(buf, "HTTP/1.0") == 0
                    || strcmp(buf, "HTTP/1.1") == 0) {
                    // ok cool
                } else {
                    // ok not cool
                    return false;
                }
                parse_state = HEADERS;
                len = 1;
            } else {
                return false;
            }
            break;
        case HEADERS:
            if (c == '\n' && len == 0) {
                // empty line, request fish
                return true;
            } else if (c == '\n') {
                len = 0;
            } else if (c != '\r') {
                len = 1;
            }
        }
    }

    return false;
}

static void updateTimeinfo()
{
    time_t now = timeClient.getEpochTime();
    localtime_r(&now, &timeinfo);
}

void loop()
{
    timeClient.update();

    updateTimeinfo();
    if (timeinfo.tm_hour >= 16 && state_of_the_dog == LILY_CHOMPED_BEFFAST) {
        state_of_the_dog = LILY_HOONGY_DINDIN;
        refresh();
    }
    if (timeinfo.tm_hour < 16 && state_of_the_dog == LILY_CHOMPED_DINDIN) {
        state_of_the_dog = LILY_HOONGY_BEFFAST;
        refresh();
    }

    static WiFiClient client;
    client = server.available();
    if (client) {
        static bool success;
        success = false;
        if (parseClientRequest(client)) {
            switch (request.kind) {
            case RequestKind::FEED:
                if (state_of_the_dog == LILY_HOONGY_BEFFAST) {
                    state_of_the_dog = LILY_CHOMPED_BEFFAST;
                    success = true;
                } else if (state_of_the_dog == LILY_HOONGY_DINDIN) {
                    state_of_the_dog = LILY_CHOMPED_DINDIN;
                    success = true;
                }
                break;
            case RequestKind::UNFEED:
                if (state_of_the_dog == LILY_CHOMPED_BEFFAST) {
                    state_of_the_dog = LILY_HOONGY_BEFFAST;
                    success = true;
                } else if (state_of_the_dog == LILY_CHOMPED_DINDIN) {
                    state_of_the_dog = LILY_HOONGY_DINDIN;
                    success = true;
                }
                break;
            }
        }
        if (success) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("que bella");
            refresh();
        } else {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("yo bro wtf is this");
        }
        client.stop();
    }
    delay(5);
}
