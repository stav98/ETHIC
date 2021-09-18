/*** LoRa WAN Lite Node ***/
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "definitions.h"
#include "functions.h"
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

void setup() 
{
  Serial.begin(115200);   //Αρχικοποίηση σειριακής
  while (!Serial);        //Περίμενε
  pinMode(STATUS_LED, OUTPUT); 
  pinMode(LED1, OUTPUT);
  blink_times_d(3, 15, 400);  //Αναβόσβησε 3 φορές γρήγορα
  //Εμφάνισε κατάσταση
  Serial.println(F("***** LoRa Remote Control System (c)2021 Stavros S. Fotoglou *****"));
  lora_init();
  randomSeed(analogRead(A3));
  //Παραγωγή κλειδιών βάσει του κλειδιού 64bit και αποθήκευση στον καθολικό πίνακα keys 16x8
  keys_generator();
  LoRa.idle();
  setupWatchDogTimer();
  sample_flag = true;
  pinMode(A2, INPUT_PULLUP);
  PCICR |= B00000010;  //Ομάδα PCINT1 για το A2 ή D16 ή PC2
  PCMSK1 |= B00000100; //Το Α2 είναι το PCINT 10
  delay(1000);
}

void loop() 
{
 //Αν δεν είναι η ώρα για μέτρηση και αποστολή φύγε χωρίς να ξυπνήσεις
 if (!f_wdt) return;   
 
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
    }
  ClassA_Dev();
}
