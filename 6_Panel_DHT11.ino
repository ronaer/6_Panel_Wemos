/*******************************************************************************
  P10 16*32 Led Matrix*6 ile nternetten saat bilgisi çekilerek yapılan saat/takvim/derece
  Internet Clock for P10 Led Matrix 16x32
  TR/izmir/ Ağustos/2022/ by Dr.TRonik YouTube

  P10 MonoColor Hardware Connections:
            ------IDC16 IN------
  CS/GPIO15/D8  |1|   |2|  D0/GPIO16
            Gnd |3|   |4|  D6/GPIO12/MISO
            Gnd |5|   |6|  X
            Gnd |7|   |8|  D5/GPIO14/CLK
            Gnd |9|   |10| D3/GPIO0
            Gnd |11|  |12| D7/GPIO13/MOSI
            Gnd |13|  |14| X
            Gnd |15|  |16| X
            --------------------

  Derleme ve karta yükleme öncesi, tüm kütüphaneler ve font dosyaları arduino ide'sine yüklenmiş olmalıdır...
*******************************************************************************/

/********************************************************************
  GLOBALS___GLOBALS___GLOBALS___GLOBALS___GLOBALS___GLOBALS___
 ********************************************************************/
//ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


//P10
#include <DMDESP.h> //https://github.com/busel7/DMDESP
#include <fonts/angka6x13.h> // Saat ve dakika için
#include <fonts/SystemFont5x7.h> // Saniye efekti için
#include <fonts/EMSans8x16.h>

//SETUP DMD
#define DISPLAYS_WIDE 6 //Yatayda 6 panel
#define DISPLAYS_HIGH 1 // Dikeyde 1 panel
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);

//İnternet üzerinden zamanı alabilme
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include "Timezone.h"

//DHT
#include <DHT.h>
#define DHTPin 3 //GPIO3 yani rx etiketli pin...
#define DHTType DHT11
DHT dht(DHTPin, DHTType);
int temp_, hum_ ;

//SSID ve Şifre Tanımlama
#define STA_SSID "Dr.TRonik"
#define STA_PASSWORD "Dr.TRonik"

//SOFT AP Tanımlama
#define SOFTAP_SSID "Saat"
#define SOFTAP_PASSWORD "87654321"
#define SERVER_PORT 2000

//Zaman intervalleri
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 300 * 1000    // In miliseconds, 5 dk da bir güncelleme
#define NTP_ADDRESS  "tr.pool.ntp.org"  // The nearest ntp pool; "north-america.pool.ntp.org" or "asia.pool.ntp.org" "tr.pool.ntp.org" etc...

WiFiServer server(SERVER_PORT);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
time_t local, utc;

bool connected = false;
unsigned long last_second;

int  p10_Brightness ;
int saat, dakika, saniye, gun, ay, yil ;

/********************************************************************
  SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___
 ********************************************************************/
void setup() {
  Serial.begin(115200);
  Disp.start();
  Disp.setBrightness(1);
  dht.begin();

  last_second = millis();

  bool r;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 35, 35), IPAddress(192, 168, 35, 35), IPAddress(255, 255, 255, 0));
  r = WiFi.softAP(SOFTAP_SSID, SOFTAP_PASSWORD, 6, 0);
  server.begin();

  timeClient.begin();   // Start the NTP UDP client

  if (r)
    Serial.println("SoftAP started!");
  else
    Serial.println("ERROR: Starting SoftAP");


  Serial.print("Trying WiFi connection to ");
  Serial.println(STA_SSID);

  WiFi.setAutoReconnect(true);
  WiFi.begin(STA_SSID, STA_PASSWORD);

  ArduinoOTA.begin();

}

/********************************************************************
  LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__
 ********************************************************************/
void loop() {
  static unsigned long timer = millis();

  Disp.loop();

  Disp.setBrightness( set_bright());

  //Handle OTA
  ArduinoOTA.handle();

  //Handle Connection to Access Point
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!connected)
    {
      connected = true;
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(STA_SSID);
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
  else
  {
    if (connected)
    {
      connected = false;
      Serial.print("Disonnected from ");
      Serial.println(STA_SSID);
    }
  }

  if (millis() - last_second > 1000)
  {
    last_second = millis();

    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();

    // convert received time stamp to time_t object

    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule usEDT = { "EDT", Second, Sun, Mar, 2, +120 };  //Eastern Daylight Time (EDT)... Türkiye: sabit +UTC+3 nedeni ile  +2saat = +120dk ayarlanmalı
    TimeChangeRule usEST = { "EST", First, Sun, Nov, 2,  };   //Eastern Time Zone: UTC - 6 hours - change this as needed//Türkiye için değil...
    Timezone usEastern(usEDT, usEST);
    local = usEastern.toLocal(utc);

    saat = hour(local); //Eğer 24 değil de 12 li saat istenirse :hourFormat12(local); olmalı
    dakika = minute(local);
    saniye = second(local);
    gun =    day(local);
    ay =     month(local);
    yil =    year(local);


  }
  // End of the millis...

  char saat_total [6];//saat verisini biçimlendirilmiş string formatına çevirmek için değişken...
  char saniye_ [3]; //saniye verisini biçimlendirilmiş string formatına çevirmek için değişken...
  char gun_ [3];
  char ay_  [3];
  char yil_ [3];
  sprintf(saat_total , "%02d:%02d", saat, dakika);
  sprintf(saniye_ , "%02d", saniye);
  sprintf(gun_ , "%02d", gun);
  sprintf(ay_ , "%02d", ay);
  sprintf(yil_ , "%02d", yil);

  Disp.setFont(EMSans8x16);
  Disp.drawText(70,  0, saat_total); //Biçimlendirilmiş char dizisini koordinatlara yazdır (x ekseni, y ekseni, string)...

  Disp.setFont(angka6x13);
  Disp.drawText(0, -1, gun_);
  Disp.drawText(18, -1, ay_);
  Disp.drawText(37, -1, yil_);

  // Dakikada bir, sıcaklık ve nem bilgisini sensörden alalım...
  if (millis() - timer > 1 * 60000) {
    timer = millis();
    dht_ ();
  }

  //Derece ve nem yazdırma:
  Disp.drawText(128, 2, String(temp_));
  Disp.drawText(160, 2, String(hum_));

  Disp.setFont(SystemFont5x7);
  Disp.drawText(112, 0, saniye_);
  Disp.drawChar(13, 6, '.');
  Disp.drawChar(32, 6, '.');
  Disp.drawText( 175, 9, "NEM:");
  Disp.drawChar(181, 0, '%');
  Disp.drawText(147, 3, "C");
  Disp.drawCircle( 144,  2,  1 );

}
//End of the loop

/********************************************************************
  VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs
********************************************************************/
int set_bright () {
  //Saate göre parlaklık  ayarlama, max254, min 1, 0 da panel göstermez...
  if (saat >= 8 && saat < 12)
  {
    p10_Brightness = 50;
  }
  else if (saat >= 12 && saat < 19)
  {
    p10_Brightness = 100;
  }
  else if (saat >= 19 && saat < 22)
  {
    p10_Brightness = 20;
  }
  else
  {
    p10_Brightness = 1;
  }
  return p10_Brightness;

}

void dht_ () {
  temp_ = dht.readTemperature();
  hum_ = dht.readHumidity();
}

/*e-posta: bilgi@ronaer.com
https://www.instagram.com/dr.tronik2022/   
YouTube: Dr.TRonik: www.youtube.com/c/DrTRonik
PCBWay: https://www.pcbway.com/project/member/shareproject/?bmbno=A0E12018-0BBC-4C
*/
