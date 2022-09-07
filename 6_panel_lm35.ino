/*******************************************************************************
  P10 16*32 Led Matrix*6 ile nternetten saat bilgisi çekilerek yapılan saat/takvim/derece
  Internet Clock for P10 Led Matrix 16x32
  TR/izmir/ Eylül/2022/ by Dr.TRonik YouTube

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

            LM35   A0

  Derleme ve karta yükleme öncesi, tüm kütüphaneler ve font dosyaları arduino ide'sine yüklenmiş olmalıdır...
*******************************************************************************/

/********************************************************************
  GLOBALS___GLOBALS___GLOBALS___GLOBALS___GLOBALS___GLOBALS___
 ********************************************************************/
//ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//P10
#include <DMDESP.h> //https://github.com/busel7/DMDESP
#include <fonts/angka6x13.h> // Derece 
#include <fonts/SystemFont5x7.h> // Saniye ve tarih
#include <fonts/EMSans8x16.h> //Saat

//SETUP DMD
#define DISPLAYS_WIDE 6 //Yatayda 6 panel
#define DISPLAYS_HIGH 1 // Dikeyde 1 panel
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);

//İnternet üzerinden zamanı alabilme
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include "Timezone.h"

//SSID ve Şifre Tanımlama
#define STA_SSID "Dr.TRonik"
#define STA_PASSWORD "Dr.TRonik"

//Zaman intervalleri
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds,  dk da bir güncelleme
#define NTP_ADDRESS  "tr.pool.ntp.org"
/* The nearest ntp pools:
  TURKİYE: "tr.pool.ntp.org"
  Worldwide: pool.ntp.org
  Asia:  asia.pool.ntp.org
  Europe:  europe.pool.ntp.org
  North America: north-america.pool.ntp.org
  Oceania: oceania.pool.ntp.org
  South America: south-america.pool.ntp.org
  etc...
*/

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
time_t local, utc;

bool connected = false;
unsigned long last_second;

int  p10_Brightness ;
int saat, dakika, saniye, gun, ay, dayWeek, yil ;

int lm35Pin = A0;
float okunan_deger = 0;
float sicaklik_gerilim = 0;
int sicaklik = 0;
int fark;
float sicaklik_ondalik;

/********************************************************************
  VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs
********************************************************************/
//Zaman zaman oluşan teardrops efektini önlemek için:
//Sırası değişmemeli...

void ICACHE_RAM_ATTR refresh() {

  Disp.refresh();
  timer0_write(ESP.getCycleCount() + 80000);

}

void Disp_init() {

  Disp.start();
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(refresh);
  timer0_write(ESP.getCycleCount() + 80000);
  interrupts();
  Disp.clear();

}

/********************************************************************
  SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___
 ********************************************************************/
void setup() {
  Serial.begin(115200);
  Disp_init();
  Disp.setBrightness(1);

  last_second = millis();

  timeClient.begin();   // Start the NTP UDP client

  Serial.print("Trying WiFi connection to ");
  Serial.println(STA_SSID);

  WiFi.setAutoReconnect(true);
  WiFi.begin(STA_SSID, STA_PASSWORD);

  lm35();

}

/********************************************************************
  LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__
 ********************************************************************/
void loop() {
  static unsigned long timer = millis();


  Disp.setBrightness( set_bright());

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

    /*_______NTPClient Tanımları__________*/

    saat    =  hour(local);   //Eğer 24 değil de 12 li saat istenirse :hourFormat12(local); olmalı
    dakika  =  minute(local);
    saniye  =  second(local);
    gun     =  day(local);
    ay      =  month(local);
    yil     =  year(local);
    dayWeek =  weekday(local); //Haftanın günleri...

    /*______NTPClient Tanımları Sonu__________*/

    //Saniye efekti_____ 1 saniyelik üst millis içinde olmazsa saniye asenkron oluyor...
    Disp.setFont(EMSans8x16);
    if (millis() / 1000 % 2 == 0) // her 1 saniye için
    {
      Disp.drawChar(88, 0, ':'); //iki noktayı göster
    }
    else
    {
      Disp.drawChar(88, 0, ' '); // gösterme
    }

  }
  // End of the millis...

  //  char saat_total [6];//saat verisini biçimlendirilmiş string formatına çevirmek için...
  char saat_ [3];   //saat verisini biçimlendirilmiş string formatına çevirmek için...
  char dakika_ [3]; //dakika verisini biçimlendirilmiş string formatına çevirmek için...
  char saniye_ [3]; //saniye verisini biçimlendirilmiş string formatına çevirmek için...
  char tarih [11];  //tarih verisini biçimlendirilmiş string formatına çevirmek için...

  //  sprintf(saat_total , "%02d:%02d", saat, dakika); //aradaki saniye efekti":" yı hareketli yapabilmek için işleme alınmadı...
  sprintf(saat_ , "%02d", saat);
  sprintf(dakika_ , "%02d", dakika);
  sprintf(saniye_ , "%02d", saniye);
  sprintf(tarih , "%02d/%02d/%02d", gun, ay, yil);

  Disp.setFont(EMSans8x16);
  //  Disp.drawText(70,  0, saat_total); //Biçimlendirilmiş char dizisini koordinatlara yazdır (x ekseni, y ekseni, string)...
  Disp.drawText(70,  0, saat_); //Biçimlendirilmiş char dizisini koordinatlara yazdır (x ekseni, y ekseni, string)...
  Disp.drawText(93,  0, dakika_);

  // Dakikada bir, sıcaklık bilgisini sensörden alalım...
  if (millis() - timer > 1 * 60000) {
    timer = millis();
    lm35();
  }

  //Derece ve nem yazdırma:
  Disp.setFont(angka6x13);
  Disp.drawText(146, 2, String(sicaklik));


  Disp.setFont(SystemFont5x7);
  Disp.drawText(112, 0, saniye_);
  Disp.drawText(2, 0, tarih);

  //
  //  Disp.drawText(152, 2, "'");
  Disp.drawText(160, 1, String(fark));
  Disp.drawCircle( 167,  3,  1 );
  Disp.drawText(170, 3, "C");

  switch (dayWeek)
  {
    case 1:
      //      Disp.drawText(17, 8, "PAZAR");
      Disp.drawText(6, 8, "  PAZAR  ");
      Disp.setPixel( 28, 15, 0 );
      Disp.setPixel( 10, 15, 0 );
      break;

    case 2:
      Disp.drawText(6, 8, "PAZARTESi");
      Disp.setPixel( 28, 15, 0 );
      Disp.setPixel( 10, 15, 0 );
      break;

    case 3:
      //      Disp.drawText(21, 8, "SALI");
      Disp.drawText(2, 8, "   SALI   ");
      Disp.setPixel( 28, 15, 0 );
      Disp.setPixel( 10, 15, 0 );
      break;

    case 4:
      Disp.drawText(8, 8, "CARSAMBA");
      Disp.setPixel( 10, 15, 1 );
      Disp.setPixel( 28, 15, 1 );
      break;

    case 5:
      Disp.drawText(8, 8, "PERSEMBE");
      Disp.setPixel( 28, 15, 1 );
      Disp.setPixel( 10, 15, 0 );
      break;

    case 6:
      // Disp.drawText(21, 8, "CUMA");
      Disp.drawText(8, 8, "  CUMA  ");
      Disp.setPixel( 28, 15, 0 );
      Disp.setPixel( 10, 15, 0 );
      break;

    case 7:
      Disp.drawText(6, 8, "CUMARTESi");
      Disp.setPixel( 28, 15, 0 );
      Disp.setPixel( 10, 15, 0 );
      break;
  }



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

void lm35() {
  okunan_deger = analogRead(lm35Pin);
  sicaklik_gerilim = (okunan_deger / 1024.0) * 3020;
  sicaklik = sicaklik_gerilim / 10.0;

  /* Virgülden sonraki bir hane için:
    Float değişkeni başka bir değişkene integer olarak atanır,
    Float değişkenden bu integer değişkeni çıkartılıp 10 ile çarpılır,
    Yeniden integer olarak dönüştürülür:
  */
  sicaklik_ondalik = sicaklik_gerilim / 10.0;
  fark = (sicaklik_ondalik - sicaklik) * 10;

}

/*e-posta: bilgi@ronaer.com
https://www.instagram.com/dr.tronik2022/   
YouTube: Dr.TRonik: www.youtube.com/c/DrTRonik
PCBWay: https://www.pcbway.com/project/member/shareproject/?bmbno=A0E12018-0BBC-4C
*/
