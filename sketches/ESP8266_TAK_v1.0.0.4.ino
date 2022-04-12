// DBG("             _ _ _ _ ||_ESP8266_TAK_ _ _ _ _ _ _");
// DBG("             \\  _  _ ||  _  _ _ ___");
// DBG("             / /__/ \\|| |_/");
// DBG("            / __   / || -  _ ___");
// DBG("           / /  / /  ||/ /");
// DBG("   _ _ _ _/ /  /  \\_/||  \\_ ______");
// DBG(" ___________\\___\\____||______________");
// DBG("");
// DBG(" ***********************************");
// DBG(" ***   ESP8266_TAK APPLICATION   ***");
// DBG(" ***********************************");
// DBG(" ***********************************");
// DBG(" *** @author: C.A Torino         ***");
// DBG(" *** @date: 12 April 2022        ***");
// DBG(" ***********************************");

/*

In the example below the trigger word "Roger" will blink the ESP8266 built in LED.
The trigger word "callRESTART" will restart the device.

*/

#include "ESP8266WiFi.h"
#include "SafeString.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESPAsyncTCP.h>

extern "C" {
#include <osapi.h>
#include <os_type.h>
}

/************************* STATIC VARS *********************************/

#define VERSION "1.0.0.4"
#define SSID "YOUR_SSID"
#define PASSWORD "YOUR_PASSWORD"
#define SERVER_HOST_NAME "YOUR_SERVER_IP"
#define TCP_PORT 8087
#define DNS_PORT 53
#define LEDPIN LED_BUILTIN
//#define LEDPIND4 D4
#define INTERVAL 1000

#define IMEI_NO 20385
#define TIME "2022-04-12T13:31:00.000Z"
#define START "2022-04-12T13:31:00.000Z"
#define STALE "2022-04-12T13:31:00.000Z"

/************************* NON STATIC VARS *********************************/

int PREV_MILLS = 0;
int LED_STATE = LOW;
int PROT = 0;
int PROT_COUNT = 0;

/************************* TIME CLIENT *********************************/

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);//+2 for South Africa

/************************* DEBUG SETUP *********************************/

#define DEBUG_ON 1
#define DEBUG_OFF 0
byte debugMode = DEBUG_ON;

#define DBG(...) debugMode == DEBUG_ON ? Serial.println(__VA_ARGS__) : NULL

/************************* INTERVALTIMER *********************************/

static os_timer_t intervalTimer;

/************************* CREATESAFESTRING *********************************/

createSafeString(msgStr, 1024);

/************************* REPLYTOSERVER METHOD *********************************/

static void replyToServer(void* arg) {
  AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);
//Serial.println(timeClient.getFormattedTime());
  if (client->space() > 1061 && client->canSend()) {
    char message[1061];//724
    if (PROT == 0) {
        PROT = 1;
        //sprintf(message, "<?xml version=\"1.0\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-%d-ping\" type=\"t-x-c-t\" time=\"%s\" start=\"%s\" stale=\"%s\" how=\"m-g\"><point lat=\"0.00000000\" lon=\"0.00000000\" hae=\"0.00000000\" ce=\"9999999\" le=\"9999999\"/><detail/></event>", IMEI_NO, TIME, START, STALE);
    } else if(PROT == 1) {
        PROT = 2;
        sprintf(message, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-%d\" type=\"a-f-G-U-C-I\" time=\"%s\" start=\"%s\" stale=\"%s\" how=\"h-g-i-g-o\"><point lat=\"0\" lon=\"0\" hae=\"0\" ce=\"9999999\" le=\"9999999\"/><detail><takv version=\"4.1.0.231\" platform=\"WinTAK-CIV\" os=\"ESP8266 OS\" device=\"ESP8266 Device using WinTAK COT\"/><contact callsign=\"callsign_%d\" endpoint=\"*:-1:stcp\"/><uid Droid=\"Droid_%d\"/><__group name=\"Red\" role=\"Team Member\"/><status battery=\"69\"/><track course=\"0.00000000\" speed=\"0.00000000\"/></detail></event>", IMEI_NO, TIME, START, STALE, IMEI_NO, IMEI_NO);
    }else if(PROT == 2) {
        PROT_COUNT++;
        sprintf(message, "<?xml version=\"1.0\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-%d-ping\" type=\"t-x-c-t\" time=\"%s\" start=\"%s\" stale=\"%s\" how=\"m-g\"><point lat=\"0.00000000\" lon=\"0.00000000\" hae=\"0.00000000\" ce=\"9999999\" le=\"9999999\"/><detail/></event>", IMEI_NO, TIME, START, STALE);
      if(PROT_COUNT == 8)
      {
        sprintf(message, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-%d\" type=\"a-f-G-U-C-I\" time=\"%s\" start=\"%s\" stale=\"%s\" how=\"h-g-i-g-o\"><point lat=\"0\" lon=\"0\" hae=\"0\" ce=\"9999999\" le=\"9999999\"/><detail><takv version=\"4.1.0.231\" platform=\"WinTAK-CIV\" os=\"ESP8266 OS\" device=\"ESP8266 Device using WinTAK COT\"/><contact callsign=\"callsign_%d\" endpoint=\"*:-1:stcp\"/><uid Droid=\"Droid_%d\"/><__group name=\"Red\" role=\"Team Member\"/><status battery=\"69\"/><track course=\"0.00000000\" speed=\"0.00000000\"/></detail></event>", IMEI_NO, TIME, START, STALE, IMEI_NO, IMEI_NO);
        PROT_COUNT = 0;
      }
    }
    client->add(message, strlen(message));
    client->send();
  }
}

/************************* HANDLEDATA METHOD *********************************/

/* event callbacks */
static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
    DBG("");
    Serial.write((uint8_t*)data, len);
    msgStr = (char*)data;
    if (msgStr.endsWith("</remarks>"))
    {
      if (msgStr.indexOf("Roger") != -1)
      {
        Blinking();
      }
    }
    if (msgStr.endsWith("</remarks>"))
    {
      if (msgStr.indexOf("callRESTART") != -1)
      {
        Restart();
      }
    }
  msgStr = "";
  os_timer_arm(&intervalTimer, 10000, true); // schedule for a reply to the server every 10s
}

/************************* ONCONNECT METHOD *********************************/

void onConnect(void* arg, AsyncClient* client) {
  DBG("Client has been connected");
  replyToServer(client);
}

/************************* ONDISCONNECT METHOD *********************************/

void onDisconnect(void* arg, AsyncClient* client) {
  DBG("Client has been Disconnected");
  Connect();
}

/************************* SETUP METHOD *********************************/

void setup() {
  pinMode(LEDPIN, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    DBG("Connecting..");
  }
  DBG("Connected to WiFi. IP:");
  DBG(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  timeClient.begin();
  Connect();
}

/************************* LOOP METHOD *********************************/

void loop(){timeClient.update();}

/************************* CONNECT METHOD *********************************/

void Connect()
{
   AsyncClient* client = new AsyncClient;
  client->onData(&handleData, client);
  client->onConnect(&onConnect, client);
  client->onDisconnect(&onDisconnect, client);
  client->connect(SERVER_HOST_NAME, TCP_PORT);
  os_timer_disarm(&intervalTimer);
  os_timer_setfn(&intervalTimer, &replyToServer, client);
  DBG("TCP client started");
}

/************************* RESTART METHOD *********************************/

void Restart()
{
  ESP.restart();
}

/************************* BLINKING METHOD *********************************/

void Blinking()
{
  unsigned long currentMillis = millis();
  if (currentMillis - PREV_MILLS >= INTERVAL) {
    PREV_MILLS = currentMillis;
    if (LED_STATE == LOW) {
      LED_STATE = HIGH;
    } else {
      LED_STATE = LOW;
    }
    digitalWrite(LEDPIN, LED_STATE);
  }
}

/************************* END *********************************/
