#include <ESP8266WiFi.h>
#include <NTPClient_Generic.h>
#include <WiFiUdp.h>
#define TIME_ZONE_OFFSET_HRS          (+3)

String curTime(void);
String curDateTime(void);
String upTime(void);
void readSettings_net(void);
void setNetVars(String, String);
void readSettings_LORA(void);
void setLoRaVars(String, String);
byte search_SF(String);
long search_BW(String);

//Ορισμός NTP Client για συγχρονισμό της ώρας
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "gr.pool.ntp.org", TIME_ZONE_OFFSET_HRS * 3600);
NTPClient timeClient(ntpUDP, TIME_ZONE_OFFSET_HRS * 3600);

//Επιστρέφει την τρέχουσα ώρα
String curTime()
{
 char t_str[9];
 if (InternAvail == "true") //Αν υπάρχει σύνδεση internet
     //Χρόνος NTP
     sprintf(t_str, "%02d:%02d:%02d", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
 else //Αλλιώς
     //Χρόνος browser
     sprintf(t_str, "%02d:%02d:%02d", hour(tim), minute(tim), second(tim));
 //return timeClient.getHours() + String(":") + timeClient.getMinutes() + String(":") + timeClient.getSeconds(); //timeClient.getFormattedDateTime();
 return String(t_str);
}

//Επιστρέφει την τρέχουσα Ημερομηνία - Ώρα
String curDateTime()
{
 char t_str[22];
 if (InternAvail == "true") //Αν υπάρχει σύνδεση internet
     //Χρόνος NTP
     sprintf(t_str, "%02d/%02d/%4d %02d:%02d:%02d", timeClient.getDay(), timeClient.getMonth(), timeClient.getYear(), timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
 else //Αλλιώς
     //Χρόνος browser
     sprintf(t_str, "%02d/%02d/%4d %02d:%02d:%02d", day(tim), month(tim), year(tim), hour(tim), minute(tim), second(tim));
 return String(t_str);
}

//Επιστρέφει τον χρόνο συνεχόμενης λειτουργίας
String upTime()
{
 char t_str[18];
 sprintf(t_str, "%dM-%dD-%02d:%02d:%02d", month(uptime)-1, day(uptime)-1, hour(uptime), minute(uptime), second(uptime));
 return String(t_str);
}

//Η συνάρτηση δέχεται String με τιμές που είναι τιμές χωρισμένες με κάποιο χαρακτήρα και  
//τις βάζει σε ένα πίνακα ακεραίων
void split(String inp, byte * out, char c)
{
 byte s = 0, idx = 0;
 //Χωρίζει το string σε αριθμούς βάσει του χαρακτήρα c
 for (byte i = 0; i < inp.length(); i++)
     {
      if (inp.substring(i, i+1) == String(c))
         {
          out[idx] = inp.substring(s, i).toInt();
          s = i + 1;
          idx++;
         }
     }
 out[idx] = inp.substring(s, inp.length()).toInt(); //Τελευταίος αριθμός μετά το delimiter
 //print_table(out, idx+1, 'd'); //Debug
}

//Ψάχνει για γνωστά DR από το json txpk του network server και επιστρέφει την τιμή SF που θα εκπέμψει στο επόμενο παράθυρο
byte search_SF(String s)
{
 String key = "SF", tmp1 = "", tmp2 = ""; //Ψάξε π.χ. στο SF9BW125 για την λέξη SF
 byte sf; //Το sf π.χ. 9
 short idx_s = s.indexOf(key); //Ψάξε για το κλειδί
 short idx_e = -1;
 if (idx_s > -1) //Αν το βρήκε
    {
     tmp1 = s.substring(idx_s + key.length()); //Πάρε από εκεί και μετά
     if (tmp1.indexOf("BW") > 0) //Αν υπάρχει η λέξη BW τότε εκεί τελειώνει η τιμή του SF
         idx_e = tmp1.indexOf("BW");
     if (idx_e > 0) //Αν τελειώνει
        {
         tmp2 = tmp1.substring(0, idx_e);
         tmp2.trim(); //Αφαίρεση κενά μπρος και πίσω
         sf = tmp2.toInt();
         return sf;
        }
     else
         return 0;
    }
 return 0;
}

//Ψάχνει για γνωστά DR από το json txpk του network server και επιστρέφει την τιμή BW που θα εκπέμψει στο επόμενο παράθυρο
long search_BW(String s)
{
 String key = "BW", tmp1 = ""; //Ψάξε π.χ. στο SF9BW125 για την λέξη BW
 byte bwK; //Το bw σε KHz π.χ. 125
 short idx_s = s.indexOf(key); //Ψάξε για το κλειδί
 if (idx_s > 0) //Αν το βρήκε
    {
     tmp1 = s.substring(idx_s + key.length()); //Πάρε από εκεί και μετά
     tmp1.trim(); //Αφαίρεση κενά μπρος και πίσω
     bwK = tmp1.toInt();
     if (bwK == 62)
         return 62500;
     else
         return bwK * 1000; //Επέστρεψε Hz π.χ. 125000 
    }
 return 0;
}

void print_table(byte *table, unsigned short len, char base)
{
 unsigned short i;
 for(i = 0; i < len-1; i++)
    {
     if (base != 'c')
        {
         if (base == 'd')
             Serial.print(table[i]);
         else if (base == 'h')
             Serial.printf("%02X", table[i]);
         Serial.print(':'); 
        }
     else
         Serial.print((char)table[i]);
    }
 if (base != 'c')
    {
     if (base == 'd')
         Serial.println(table[i]);
     else if (base == 'h')
         Serial.printf("%02X\n", table[i]);
    }
 else
    Serial.println((char)table[i]);
}

void blink_led(byte pin)
{
 if (millis() - blink_per > 100)
    {
     byte out = led_pat & 1;
     digitalWrite(pin, out); //Ανάβει
     led_pat >>= 1;
     led_pat += out * 512;
     blink_per = millis();
    }
}

//Διαβάζει τις αποθηκευμένης ρυθμίσεις του αρχείου και τις βάζει στα controls ρύθμισης δικτύου 
//της σελίδας setup_net.html. Καλείται κατά το ξεκίνημα και μετά την αποθήκευση.
void readSettings_net()
{
 String buffer;
 File file = LittleFS.open("/settings_net.txt", "r");
 //Αν το αρχείο δεν υπάρχει (πρώτη φορά) τότε να βάλει κάποιες προκαθορισμένες τιμές
 if (!file) 
    {
     Serial.println(F("Failed to open file /settings_net.txt for reading. Loading Defaults ..."));
     //Βάλε αρχικές τιμές στις μεταβλητές
     SetAP = "false"; //<--
     AP_SSID = "LORA_Lite_GW1";
     AP_CHANNEL = "6"; 
     AP_ENCRYPT = "false";
     AP_KEY = ""; 
     SetSTA = "true"; //<--
     ST_SSID = "SSID";
     ST_KEY = "KEY";
     IP_STATIC = "true"; //<--
     IP_ADDR = "192.168.1.100"; //192.168.1.100
     IP_MASK = "255.255.255.0";
     IP_GATE = "192.168.1.1"; //192.168.1.1
     IP_DNS = "192.168.1.1"; //192.168.1.1
     NTP_TZ = TIME_ZONE_OFFSET_HRS;
     BROW_TO = TIME_ZONE_OFFSET_HRS;
     NTP_SRV = "gr.pool.ntp.org";
     NET_SRV = "192.168.1.45";
     NET_SRV_PORT = "1700";
     MQTT_USER = "";
     MQTT_PASS = "";
     MQTT_CLIENT = "Therm";
     LOG_NAME = "admin";
     LOG_PASS = "admin";
     //Γράψε το αρχείο την πρώτη φορά μόνο
     File file = LittleFS.open("/settings_net.txt", "w");
     if (!file) 
        {
         Serial.println(F("Error opening file for writing"));
         return;
        }
     file.print(F("#Automatic created from script\n"));
     file.print("N1::" + SetAP + "\n");
     file.print("N2::" + AP_SSID + "\n");
     file.print("N3::" + AP_CHANNEL + "\n");
     file.print("N4::" + AP_ENCRYPT + "\n");
     file.print("N5::" + AP_KEY + "\n");
     file.print("N6::" + SetSTA + "\n");
     file.print("N7::" + ST_SSID + "\n");
     file.print("N8::" + ST_KEY + "\n");
     file.print("N9::" + IP_STATIC + "\n");
     file.print("N10::" + IP_ADDR + "\n");
     file.print("N11::" + IP_MASK + "\n");
     file.print("N12::" + IP_GATE + "\n");
     file.print("N13::" + IP_DNS + "\n");
     file.print("N14::" + NTP_TZ + "\n");
     file.print("N15::" + BROW_TO + "\n");
     file.print("N16::" + NTP_SRV + "\n");
     file.print("N17::" + NET_SRV + "\n");
     file.print("N18::" + NET_SRV_PORT + "\n");
     file.print("N19::" + MQTT_USER + "\n");
     file.print("N20::" + MQTT_PASS + "\n");
     file.print("N21::" + MQTT_CLIENT + "\n");
     file.print("N22::" + LOG_NAME + "\n");
     file.print("N23::" + LOG_PASS + "\n");
     file.close();
     return;
    }
 String value1, value2;
 while (file.available()) 
       {
        buffer = file.readStringUntil('\n');
        //Serial.println(buffer); //Debug  
        if (buffer.indexOf("::") > 0)
           {
            for (unsigned int i = 0; i < buffer.length(); i++) 
                {
                 if (buffer.substring(i, i+2) == "::") 
                    {
                     value1 = buffer.substring(0, i);
                     value2 = buffer.substring(i + 2);
                     break;
                    }
                }
            setNetVars(value1, value2);
           }
       }
 file.close();
}

//Ανάλογα με το κλειδί του αρχείου N1, N2, ... Nn, βάλε την τιμή στην αντίστοιχη μεταβλητή ώστε να εμφανιστούν οι
//τιμές στην αρχική σελίδα αφού περάσουν από τον processor.
void setNetVars(String key, String value)
{
 if (key == "N1") SetAP = value;
 else if (key == "N2") AP_SSID = value;
 else if (key == "N3") AP_CHANNEL = value;
 else if (key == "N4") AP_ENCRYPT = value;
 else if (key == "N5") AP_KEY = value;
 else if (key == "N6") SetSTA = value;
 else if (key == "N7") ST_SSID = value;
 else if (key == "N8") ST_KEY = value;
 else if (key == "N9") IP_STATIC = value;
 else if (key == "N10") IP_ADDR = value;
 else if (key == "N11") IP_MASK = value;
 else if (key == "N12") IP_GATE = value;
 else if (key == "N13") IP_DNS = value;
 else if (key == "N14") NTP_TZ = value;
 else if (key == "N15") BROW_TO = value;
 else if (key == "N16") NTP_SRV = value;
 else if (key == "N17") NET_SRV = value;
 else if (key == "N18") NET_SRV_PORT = value;
 else if (key == "N19") MQTT_USER = value;
 else if (key == "N20") MQTT_PASS = value;
 else if (key == "N21") MQTT_CLIENT = value;
 else if (key == "N22") LOG_NAME = value;
 else if (key == "N23") LOG_PASS = value;
}

//Διαβάζει τις αποθηκευμένης ρυθμίσεις του αρχείου και τις βάζει στα controls ρύθμισης δικτύου 
//της σελίδας setup_radio.html. Καλείται κατά το ξεκίνημα και μετά την αποθήκευση.
void readSettings_LORA()
{
 String buffer;
 File file = LittleFS.open("/settings_lora.txt", "r");
 //Αν το αρχείο δεν υπάρχει (πρώτη φορά) τότε να βάλει κάποιες προκαθορισμένες τιμές
 if (!file) 
    {
     Serial.println(F("Failed to open file /settings_lora.txt for reading. Loading Defaults ..."));
     //Βάλε αρχικές τιμές στις μεταβλητές
     lora_freq = String(FREQUENCY);
     lora_sf = String(SF);
     lora_cr = String(CR);
     lora_bw = String(BW);
     lora_sync = String(SYNCWORD);
     lora_power = String(POWER);
     
     //Γράψε το αρχείο την πρώτη φορά μόνο
     File file = LittleFS.open("/settings_lora.txt", "w");
     if (!file) 
        {
         Serial.println(F("Error opening file for writing"));
         return;
        }
     file.print(F("#Automatic created from script\n"));
     file.print("L1::" + lora_freq + "\n");
     file.print("L2::" + lora_sf + "\n");
     file.print("L3::" + lora_cr + "\n");
     file.print("L4::" + lora_bw + "\n");
     file.print("L5::" + lora_sync + "\n");
     file.print("L6::" + lora_power + "\n");
     file.close();
     return;
    }
 String value1, value2;
 while (file.available()) 
       {
        buffer = file.readStringUntil('\n');
        //Serial.println(buffer); //Debug  
        if (buffer.indexOf("::") > 0)
           {
            for (unsigned int i = 0; i < buffer.length(); i++) 
                {
                 if (buffer.substring(i, i+2) == "::") 
                    {
                     value1 = buffer.substring(0, i);
                     value2 = buffer.substring(i + 2);
                     break;
                    }
                }
            setLoRaVars(value1, value2);
           }
       }
 file.close();
}

//Ανάλογα με το κλειδί του αρχείου N1, N2, ... Nn, βάλε την τιμή στην αντίστοιχη μεταβλητή ώστε να εμφανιστούν οι
//τιμές στην αρχική σελίδα αφού περάσουν από τον processor.
void setLoRaVars(String key, String value)
{
 if (key == "L1") lora_freq = value;
 else if (key == "L2") lora_sf = value;
 else if (key == "L3") lora_cr = value;
 else if (key == "L4") lora_bw = value;
 else if (key == "L5") lora_sync = value;
 else if (key == "L6") lora_power = value;
}
