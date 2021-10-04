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
// DBG(" *** @initial_date: 23 July 2021 ***");
// DBG(" ***********************************");

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

#define VERSION "1.0.0.2"
#define SSID "YOUR_SSID"
#define PASSWORD "YOUR_PASSWORD"
#define SERVER_HOST_NAME "YOUR_SERVER_IP"
#define TCP_PORT 8087
#define DNS_PORT 53
#define LEDPIN LED_BUILTIN
//#define LEDPIND4 D4
#define INTERVAL 1000

int PREV_MILLS = 0;
int LED_STATE = LOW;
int PROT = 0;
int PROT_COUNT = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);//+2 for South Africa

/************************* DEBUG Setup *********************************/

#define DEBUG_ON 1
#define DEBUG_OFF 0
byte debugMode = DEBUG_ON;

#define DBG(...) debugMode == DEBUG_ON ? Serial.println(__VA_ARGS__) : NULL

/************************* intervalTimer *********************************/

static os_timer_t intervalTimer;

/************************* createSafeString *********************************/

createSafeString(msgStr, 1024);

/************************* replyToServer METHOD *********************************/

static void replyToServer(void* arg) {
  AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);
//Serial.println(timeClient.getFormattedTime());
  // send reply
  if (client->space() > 1061 && client->canSend()) {
    char message[1061];//724
    //sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
    if (PROT == 0) {
        PROT = 1;
        sprintf(message, "<?xml version=\"1.0\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-20002-ping\" type=\"t-x-c-t\" time=\"2021-10-03T13:31:00.000Z\" start=\"2021-10-03T13:31:00.000Z\" stale=\"2021-10-04T00:50:00.000Z\" how=\"m-g\"><point lat=\"0.00000000\" lon=\"0.00000000\" hae=\"0.00000000\" ce=\"9999999\" le=\"9999999\"/><detail/></event>");
    } else if(PROT == 1) {
        PROT = 2;
        sprintf(message, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-20002\" type=\"a-f-G-U-C-I\" time=\"2021-10-03T13:31:00.000Z\" start=\"2021-10-03T13:31:00.000Z\" stale=\"2021-10-04T00:50:00.000Z\" how=\"h-g-i-g-o\"><point lat=\"0\" lon=\"0\" hae=\"0\" ce=\"9999999\" le=\"9999999\"/><detail><takv version=\"4.1.0.231\" platform=\"WinTAK-CIV\" os=\"Microsoft Windows 10 Pro\" device=\"System manufacturer System Product Name\"/><contact callsign=\"callsign_00002\" endpoint=\"*:-1:stcp\"/><uid Droid=\"Droid_00002\"/><__group name=\"Red\" role=\"Team Member\"/><status battery=\"100\"/><track course=\"0.00000000\" speed=\"0.00000000\"/></detail></event>");
    }else if(PROT == 2) {
        PROT_COUNT++;
        sprintf(message, "<?xml version=\"1.0\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-20002-ping\" type=\"t-x-c-t\" time=\"2021-10-03T13:31:00.000Z\" start=\"2021-10-03T13:31:00.000Z\" stale=\"2021-10-04T00:50:00.000Z\" how=\"m-g\"><point lat=\"0.00000000\" lon=\"0.00000000\" hae=\"0.00000000\" ce=\"9999999\" le=\"9999999\"/><detail/></event>");
      if(PROT_COUNT == 8)
      {
        sprintf(message, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?><event version=\"2.0\" uid=\"S-1-5-21-1568504889-667903775-1938598950-20002\" type=\"a-f-G-U-C-I\" time=\"2021-10-03T13:31:00.000Z\" start=\"2021-10-03T13:31:00.000Z\" stale=\"2021-10-04T00:50:00.000Z\" how=\"h-g-i-g-o\"><point lat=\"0\" lon=\"0\" hae=\"0\" ce=\"9999999\" le=\"9999999\"/><detail><takv version=\"4.1.0.231\" platform=\"WinTAK-CIV\" os=\"Microsoft Windows 10 Pro\" device=\"System manufacturer System Product Name\"/><contact callsign=\"callsign_00002\" endpoint=\"*:-1:stcp\"/><uid Droid=\"Droid_00002\"/><__group name=\"Red\" role=\"Team Member\"/><status battery=\"100\"/><track course=\"0.00000000\" speed=\"0.00000000\"/></detail></event>");
        PROT_COUNT = 0;
      }
    }
    client->add(message, strlen(message));
    client->send();
  }
}

/************************* handleData METHOD *********************************/

/* event callbacks */
static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
    Serial.printf("\n data received from %s \n", client->remoteIP().toString().c_str());
    Serial.write((uint8_t*)data, len);
    msgStr = (char*)data;
    if (msgStr.endsWith("</remarks>"))
    {
      if (msgStr.indexOf("Roger") != -1)
      {
        //Serial.printf("\n blinking SUCCESS,0$ \n");
        blinking();
      }
    }
    if (msgStr.endsWith("</remarks>"))
    {
      if (msgStr.indexOf("callRESTART") != -1)
      {
        blinking();
      }
    }
  msgStr = "";
  os_timer_arm(&intervalTimer, 10000, true); // schedule for a reply to the server every 10s
}

/************************* onConnect METHOD *********************************/

void onConnect(void* arg, AsyncClient* client) {
  Serial.printf("\n client has been connected to %s on port %d \n", SERVER_HOST_NAME, TCP_PORT);
  replyToServer(client);
}

/************************* onDisconnect METHOD *********************************/

void onDisconnect(void* arg, AsyncClient* client) {
  Serial.printf("\n client has been Disconnected from %s on port %d \n", SERVER_HOST_NAME, TCP_PORT);
  Connect();
}

/************************* setup METHOD *********************************/

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

/************************* loop METHOD *********************************/

void loop(){timeClient.update();}

/************************* Connect METHOD *********************************/

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

/************************* Restart METHOD *********************************/

void Restart()
{
  ESP.restart();
}

/************************* blinking METHOD *********************************/

void blinking()
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
