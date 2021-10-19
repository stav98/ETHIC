/*** LoRa WAN Lite Node ***/
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
//#include <avr/wdt.h>
#include "definitions.h"
#include "thermo.h"
#include "functions.h"
#include "cmd.h"
#include "commands.h"
#include "encrypt.h"
#include "lora.h"
#include "ClassA_Dev.h"
#include "base64.hpp"

//Μήνυμα έως 128 χαρακτήρες //230
//------------          1         2         3         4         5         6         7         8         9        10        11        12        13        14        15        16        17        18        19        20        21        22        23                                                                                                
//------------ 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
//char m[] =    "@*Hello world 12345678 preveza prevezas stavros1234 -----1 sdsdd fgfgfg   rrrtt    ~~~~~~``````m mmm ##### $$$$$$$$$$$ ><><>---."; //Χρήση Character Array
char m[] =    "Test Preveza: ";
//byte yy;
String tmp_str;
unsigned long cnt; //Μετρητής για την γραμμή μηνύματος -θα φύγει στο τελικό

ISR (PCINT1_vect) 
{
 PCMSK1 &= B11111011;
 f_wdt = true;
 PCMSK1 |= B00000100;
}

void win1_open()
{
  if (readHC597() & (1 << DIN_W1T1))
     {
     W1_OPEN();
     analogWrite(WINDOW_SPEED_PIN, 180);
     }
  else
     {
      W1_STOP();
      analogWrite(WINDOW_SPEED_PIN, 0);
     }
 writeHC595(dout_status);
}

void win1_close()
{
  if (readHC597() & (1 << DIN_W1T2))
     {
     W1_CLOSE();
     analogWrite(WINDOW_SPEED_PIN, 180);
     }
  else
     {
      W1_STOP();
      analogWrite(WINDOW_SPEED_PIN, 0);
     }
 writeHC595(dout_status);
}

void win2_open()
{
  if (readHC597() & (1 << DIN_W2T1))
     {
     W2_OPEN();
     analogWrite(WINDOW_SPEED_PIN, 180);
     }
  else
     {
      W2_STOP();
      analogWrite(WINDOW_SPEED_PIN, 0);
     }
 writeHC595(dout_status);
}

void win2_close()
{
  if (readHC597() & (1 << DIN_W2T2))
     {
     W2_CLOSE();
     analogWrite(WINDOW_SPEED_PIN, 180);
     }
  else
     {
      W2_STOP();
      analogWrite(WINDOW_SPEED_PIN, 0);
     }
 writeHC595(dout_status);
}

void skia_open()
{
  if (readHC597() & (1 << DIN_SKT1))
     {
     SKIA_OPEN();
     //analogWrite(WINDOW_SPEED_PIN, 200);
     }
  else
     {
      SKIA_STOP();
      //analogWrite(WINDOW_SPEED_PIN, 0);
     }
 writeHC595(dout_status);
}

void skia_close()
{
  if (readHC597() & (1 << DIN_SKT2))
     {
     SKIA_CLOSE();
     //analogWrite(WINDOW_SPEED_PIN, 200);
     }
  else
     {
      SKIA_STOP();
      //analogWrite(WINDOW_SPEED_PIN, 0);
     }
 writeHC595(dout_status);
}

void setup() 
{
  //Serial.begin(115200);   //Αρχικοποίηση σειριακής
  cmdInit(115200);
  while (!Serial);        //Περίμενε
  pinMode(FAN_PIN, OUTPUT);
  pinMode(SCLK_PIN, OUTPUT);
  pinMode(STCP_PIN, OUTPUT);
  pinMode(P_LOAD, OUTPUT);
  pinMode(SDATA_IN_PIN, INPUT);
  pinMode(STATUS_LED, OUTPUT); 
  pinMode(LED1, OUTPUT);
  blink_times_d(3, 15, 400);  //Αναβόσβησε 3 φορές γρήγορα
  //Εμφάνισε κατάσταση
  Serial.println(F("***** ETHIC (c)2021 Stavros S. Fotoglou *****"));
  lora_init();
  randomSeed(analogRead(A2));
  //Παραγωγή κλειδιών βάσει του κλειδιού 64bit και αποθήκευση στον καθολικό πίνακα keys 16x8
  keys_generator();
  LoRa.idle();
  //setupWatchDogTimer();
  sample_flag = true;
  init_thermo();  //Αρχικοποίηση θερμομέτρων 1-wire
  dht.begin();    //Αρχικοποίηση DHT22
  writeHC595(0);  //Κάνε Off όλες τις εξόδους αν έχει σκουπίδια μετά το reset
  add_commands(); //Πρόσθεσε εντολές
  //analogWrite(FAN_PIN, 80); //80 - 200
  delay(1000);
  interval = millis();
}

void loop() 
{
 //Αν δεν είναι η ώρα για μέτρηση και αποστολή φύγε χωρίς να ξυπνήσεις
 cmdPoll();
 //Serial.println(readHC597());
 //win1_open();
 //win1_close();

 //win2_open();
 //win2_close();
 
 
 //skia_close();
 //skia_open();
 //if (!f_wdt) return;   
 if ((millis() - interval) >= INTERVAL_MS)
    {
     sample_flag = true;
     f_wdt = true;
     interval = millis();
    }
 //===== Εδώ γίνονται οι μετρήσεις π.χ. θερμοκρασίας υγρασίας κλπ. και ετοιμάζεται το μήνυμα MQTT ========
 if (sample_flag)
    {
     tmp_str = m;
     tmp_str += cnt;
     //m = tmp_str.c_str();
     payload_len = encrypt((byte*)tmp_str.c_str(), (byte*)out); //Κρυπτογράφησε με DES
     //raw(m, out); //ή ετοίμασε μη κρυπτογραφημένα δεδομένα
     //Serial.println(out); //Debug
     //Serial.println(yy); //Debug   
     //decrypt((byte*)out, oo, yy); //Debug για δοκιμή αλγορίθμου
     cnt++;
     sample_flag = false;
     //Serial.println(readHC597());
     ReadSensors();
     Serial.println(((882 - analogRead(PH_PIN)) * 0.012) + 2.5); //ph
     Serial.println(analogRead(IRRAD_PIN));
    }
 if (f_wdt)
 {
 ClassA_Dev();
 }
 //sample_flag = true; 
 //f_wdt = false;
 //Serial.println(readHC597());
 //digitalWrite(STCP_PIN,HIGH);
 //delay(5);
 //digitalWrite(STCP_PIN,LOW);
 //delay(1000);
 //ReadSensors();
 //Serial.println((float)(5 * analogRead(A0)) / 1023);
 //delay(200);
}
