//pio run --target uploadfs
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#include <SPI.h>
#include <LoRa.h>
#include "definitions.h"
#include "base64.hpp"
#include "functions.h"
#include "lora.h"
#include "network.h"

//Ορισμός Timer
static os_timer_t intervalTimer;

byte txbuf[256];      //Η buffer με το μήνυμα LoRa προς εκπομπή
byte tmp_buff[512];
String tmp1, data;
int frame_len = 0;
byte ptr1, ptr2;

void setup() 
{
 pinMode(BUILTIN_LED, OUTPUT);
 digitalWrite(BUILTIN_LED, HIGH);
 Serial.begin(115200);
 Serial.println();
 Serial.println(F("========== LoRa Lite Gateway (c)2022 by Stavros S. Fototglou [SV6GMP] =========="));
 //Προετοιμασία συστήματος αρχείων 3 Mbyte
 if (!LittleFS.begin()) 
    {
     Serial.println("LittleFS initialization failed!");
     while (1) yield(); // Stay here twiddling thumbs waiting
    }
 Serial.println(F("\r\nLittleFS available!"));
 LittleFS.begin();
 //Αν ξεκίνησε το σύστημα αρχείων στην εσωτερική Flash 4Mbyte
 //Εμφάνισε ελεύθερο χώρο από το σύστημα SPIFFS
 FSInfo fs_info;
 LittleFS.info(fs_info);
 float fileTotalKB = (float)fs_info.totalBytes / 1024.0; 
 float fileUsedKB = (float)fs_info.usedBytes / 1024.0; 
 char tmp[40];
 sprintf(tmp, "Used %.1fKb of total %.1fKb", fileUsedKB, fileTotalKB);
 Serial.println(tmp);
 readSettings_net(); //Διάβασε ρυθμίσεις δικτύου από αρχείο
 readSettings_LORA(); //Διάβασε ρυθμίσεις LoRa από αρχείο
 //Ρουτίνα εξυπηρέτησης αν γίνει σύνδεση TCP με τον server π.χ. www.google.com 
 test_client->onConnect(&onConnect, test_client); //Στο network.h
 led_pat = 0b0111111111; //Αρχική κατάσταση
 initNetwork(); //Αρχικοποίηση δικτύου WiFi, IP, NTP και WebServer
 blink_per = millis();
 initLoRa(); //Αρχικοποίηση πομποδέκτη LoRa
 pull_timer = millis(); 
 txbuf[0] = '\0'; //Αρχικά άδειο buffer εκπομπής
 tx_wait = true; //Αρχικά περιμένει να πάρει χρόνο tmst από τον NW Server
 //tx_tmst = 4294967295; //Αρχικά το timestamp είναι μεγάλο ώστε να μην κάνει εκπομπή σε TX window έως να πάρει τιμή από τον NW Server
 //packets[0].tx_tmst=2345;
 ptr1 = ptr2 = 0;
 //Κάθε 30 sec κάνε έλεγχο σύνδεσης
 os_timer_disarm(&intervalTimer); //Απενεργοποίηση Timer
 os_timer_setfn(&intervalTimer, &testcon, NULL); //Ορισμός ρουτίνας εξυπηρέτησης χωρίς ορίσματα στο network.h
 os_timer_arm(&intervalTimer, 30000, true);  //Ενεργοποίηση με επανάληψη κλήσης κάθε 30 sec
 uptime = 0;
}

void loop() 
{
 if (InternAvail == "true")
     timeClient.update(); //Όταν φτάσει η στιγμή ανανέωσε την ώρα από τον NTP Server
 //Αν πέρασε ~1sec μετά την λήψη δηλ. το παράθυρο RX1 ή RX2 (2sec) τότε στείλε τα δεδομένα στο Node
 if ((micros() >= (packets[ptr2].tx_tmst - PRE_OPEN_TIME)) && (tx_state == TX) && (ptr1 != ptr2)) //996400
     {
      Serial.print("------");
      Serial.println(packets[ptr2].tx_tmst);
      LoRa.setSpreadingFactor(packets[ptr2].sf); //Βάλε το SF στη τιμή που ζήτησε ο NW Server
      LoRa.setSignalBandwidth(packets[ptr2].bw); //Βάλε το BW στη τιμή που ζήτησε ο NW Server
      //* Κατάσταση εκπομπής *
      if (packets[ptr2].frame_len > 0)
         {
          if (packets[ptr2].ipol == "true") //Αν το θέλει ο NW Server
              LoRa.enableInvertIQ(); //Για εκπομή από πύλη προς Node θα αντιστρέψει τα I/Q ώστε να μην ακούει η μια πύλη την άλλη
          LoRa.beginPacket(); //Ετοίμασε πακέτο LoRa προς αποστολή στο Node
          LoRa.write(packets[ptr2].txbuf, frame_len); 
          LoRa.endPacket();                     // Κλείσε το πακέτο και στείλτο
          //Φτιάχνει ένα string με πληροφορίες για το πακέτο που έλαβε ώστε να εμφανιστεί στο web dashboard
          unsigned short frameId = packets[ptr2].txbuf[4] * 256 + packets[ptr2].txbuf[3]; 
          sprintf(tx_pkt_log, "%s | %02X%02X | %04X | %03d <br>\n",curTime().c_str(), packets[ptr2].txbuf[2], packets[ptr2].txbuf[1], frameId, packets[ptr2].frame_len);
          txpkt_cnt++;
          //Μετακίνησε τις γραμμές κατά μια θέση σβήνοντας την παλαιότερη γραμμή
          for (byte i = 0; i < LOG_LINES-1; i++)
               tx_pkts_log[i] = tx_pkts_log[i+1];
          //Βάλε στην τελευταία γραμμή τα νεότερα δεδομένα
          tx_pkts_log[LOG_LINES-1] = String(tx_pkt_log);
          //Φτιάξε το τελικό string το οποίο θα στέλνει στο web interface
          tx_pkts_str = "";
          for (byte i = 0; i < LOG_LINES; i++)
               tx_pkts_str += tx_pkts_log[i];     
         }
      LoRa.disableInvertIQ(); //Στο τέλος της εκπομπής τα I/Q γυρίζουν κανονικά
      LoRa.setSpreadingFactor(lora_sf.toInt()); //Γύρισε πάλι στο προκαθορισμένο SF
      LoRa.setSignalBandwidth(lora_bw.toInt()); //Γύρισε πάλι στο προκαθορισμένο BW
      LoRa.receive(); //Επιστροφή σε κατάσταση λήψης
      tx_state = RX;
      Serial.println(F("* Transmit to Node ->"));
      ptr2++;
      if (ptr2 >= BUFF_LEN)
          ptr2 = 0;
     }
 
 //Κάθε 30sec στείλε πακέτο PULL στον NW Server
 if (millis() - pull_timer > PULLDATA_INTERVAL)
    {
     pull_data();
     pull_timer = millis();
     NWSrv_State = "false"; //Αφού στείλει Pull, θεώρησε ότι είναι offline και αν πάρει απάντηση θα γίνει true
     uptime += 30; //Ενημέρωσε το uptime κατά 30 sec
     tim += 30; //Ενημέρωσε το epoch time κατά 30 sec αν δεν λειτουργεί το NTP
     if (WiFi.status() == WL_CONNECTED)
         S_Strength = String(WiFi.RSSI()) + "dBm";
     else
         S_Strength = "";
    }

 //Συνεχόμενα έλεγξε για εισερχόμενο πακέτο από τον NW Server
 int packetSize = UDP.parsePacket();
 //Αν υπάρχει πακέτο
 if (packetSize)
    {
     // receive incoming UDP packets
     Serial.printf("Received %d bytes from %s, port %d: ", packetSize, UDP.remoteIP().toString().c_str(), UDP.remotePort());
     NWSrv_State = "true"; //Για να ανάψει ειδοποίηση στο web status bar
     int len = UDP.read(buff_down, 512);
     if (len > 0)
         buff_down[len] = 0; //Βάλε χαρακτήρα τερματισμού /0 στο buffer
     //print_table(buff_down, len, 'h'); //c Debug εμφανίζει δεδομένα πακέτου λήψης στο δυαδικό
     //Serial.printf("UDP packet contents: %s\n", incomingPacket);
     if (buff_down[3] == 4) //Αν είναι 4 τότε έχουμε Pull Ack από NW Server
         Serial.println(F("Pull ACK OK"));
     //Αν είναι 1 και έχει το ίδιο και έχει το ίδιο ID με το τελευταίο που έγινε push τότε είναι σωστό Push Ack 
     if (buff_down[3] == 1 && buff_down[1] == last_h1 && buff_down[2] == last_h2)
         Serial.println(F("Push ACK OK"));
     if (buff_down[3] == 3) //Αν είναι 3 τότε έχουμε Pull Resp από NW Server
        {
         Serial.println(F("Pull Resp from NW Server"));
         unsigned short j=0;
         //Πάρε μόνο το αντικείμενο JSON (αφαίρεσε τα 4 πρώτα bytes)
         for (unsigned short i=4; i < sizeof(buff_down); i++)
             {
              tmp_buff[j] = buff_down[i];
              j++;
             }
         tmp1 = String((char*)tmp_buff); //Μετατροπή από byte array σε String
         data = parse_json(tmp1, "data"); //Πάρε το περιεχόμενο του κλειδιού data
         ipol = parse_json(tmp1, "ipol");
         sf = search_SF(parse_json(tmp1, "datr")); //Το κλειδί datr π.χ. SF9BW125 και παίρνει την τιμή του SF
         bw = search_BW(parse_json(tmp1, "datr")); //Το κλειδί datr π.χ. SF9BW125 και παίρνει την τιμή του BW
         size = (byte) parse_json(tmp1, "size").toInt();
         tx_tmst = (unsigned long) strtoul(parse_json(tmp1, "tmst").c_str(), NULL, 10); //Πάρε το timestamp από json
         if (micros() < tx_tmst) //Αν ο χρόνος είναι μικρότερος από τον τρέχοντα άργησε το udp πακέτο και να περιμένει το επόμενο
            {
             packets[ptr1].tx_tmst = tx_tmst;
             Serial.print("tmst: ===========> "); //Debug
             Serial.println(packets[ptr1].tx_tmst); //Debug
             char a[256];
             data.toCharArray(a, 256); //Μετατροπή από String σε Char Array
             frame_len = decode_base64((unsigned char*) a, (unsigned char*) packets[ptr1].txbuf); //Αποκωδικοποίηση BASE64 και να πάει στο txbuf
             packets[ptr1].frame_len = frame_len;
             packets[ptr1].ipol = ipol;
             packets[ptr1].sf = sf;
             packets[ptr1].bw = bw;
             print_table((byte*) packets[ptr1].txbuf, frame_len, 'h');
             Serial.println("TX_ACK >");
             tx_ack(); //Απάντησε στον NW server
             ++ptr1;
             if (ptr1 >= BUFF_LEN)
                 ptr1 = 0;
            }
        }
    }
 blink_led(BUILTIN_LED);
}