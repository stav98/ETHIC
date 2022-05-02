#include <NTPClient_Generic.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "ESPAsyncWebServer.h"

//static void handleData(void*, AsyncClient*, const void *, size_t);
void readFileNames(byte*, byte*);
void chk_Dl(void);
static void rst(void*);
//void callback(String, byte*, unsigned int);
//void reconnect(void);
void initNetwork(void);
void startWiFi(void);
void notFound(AsyncWebServerRequest*);
String processor(const String&);
String processor_net(const String&);
String ChartData(byte, bool);
void saveSettings(AsyncWebServerRequest*, char*);
static void testcon(void*);
void onConnect(void*, AsyncClient*);
//void onConnectUpd(void*, AsyncClient*); //Η συνάρτηση καλείται όταν γίνει σύνδεση στον update host
//void download(char*); //Κάνει αίτημα για σύνδεση στον Update Host

//Δημιουργία αντικειμένων Server και Clients
AsyncWebServer server(80); //Ο τοπικός web server
AsyncClient* test_client = new AsyncClient; //Δοκιμαστική σύνδεση TCP για έλεγχο σύνδεσης με διαδίκτυο
AsyncClient* update_client = new AsyncClient; //Σύνδεση στον Update Host για κατέβασμα αναβαθμήσεων

int chunks = 0;
bool dl_block = false;
unsigned long dl_delay;

byte buff_down[512]; 
byte buff_up[1024];  
unsigned short buff_index = 0;

char reply[] = "Packet received!";

//Ορισμός αντικειμένου UDP
WiFiUDP UDP;

static os_timer_t restartDelay;

//Επανεκκίνηση του ESP
static void rst(void* arg)
{
 os_timer_disarm(&restartDelay);
 ESP.restart();
}

uint8_t MAC_array[6];
char MAC_char[19];  

void initNetwork()
     {
      startWiFi();
      
      //Αρχικά κάνε σύνδεση με τον testhost
      test_client->connect(testhost, 80);
      delay(1000); //Περίμενε λίγο //500
      testcon(NULL); //Δες αν έγινε η σύνδεση
      
      //Ξεκίνησε τον NTP Client
      const char *ntp_name;
      ntp_name = NTP_SRV.c_str(); //Μετατροπή του String σε const char*
      //Άλλαξε το όνομα του Time Server
      timeClient.setPoolServerName(ntp_name);
      timeClient.setTimeOffset(NTP_TZ.toInt() * 3600); //Άλλαξε την ζώνη ώρας  
      timeClient.begin();
      timeClient.setUpdateInterval(SECS_IN_DAY); //Αν καλείται στο Loop θα κάνει update μια φορά την μέρα SECS_IN_DAY ή κάθε ώρα SECS_IN_HR
      //Serial.println(timeClient.getPoolServerName()); //Debug
      bool ntp_OK = false;
      byte ntp_tries = 0;
      if (InternAvail == "true")
         {
          while (!ntp_OK && ntp_tries < 20) //10
                {
                 timeClient.update();
                 delay(100);
                 if (timeClient.updated())
                     ntp_OK = true;
                 ntp_tries++;
                 delay(100);
                }
         }
      if (ntp_OK)
          Serial.println(F("Time Client Updated"));
      else
          Serial.println(F("Time Client Not Updated"));
      //Serial.println("UTC : " + timeClient.getFormattedUTCDateTime()); //UTC
      Serial.println("LOC : " + timeClient.getFormattedDateTime()); //LOCAL

      //=====================================================  Συναρτήσεις χειρισμού του Web Server ======================================================================
      //Ζητήθηκε η αρχική σελίδα /
      server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
               {
                if (!request->authenticate(LOG_NAME.c_str(), LOG_PASS.c_str()))
                    return request->requestAuthentication();
                else
                    request->send(LittleFS, "/index.html", String(), false, processor); //Χρήση processor αν στο index.html υπάρχει %ΛΕΞΗ%, τότε θα βάλει στη θέση ότι επιστρέψει η processor σαν ASP
                    //request->send(SPIFFS, "/index.html", "text/html"); //Χωρίς χρήση processor
               });
      //Ζητήθηκε η υποσελίδα /setup_wp.html
      server.on("/setup_radio.html", HTTP_GET, [](AsyncWebServerRequest * request)
               {
                if (!request->authenticate(LOG_NAME.c_str(), LOG_PASS.c_str()))
                    return request->requestAuthentication();
                else
                    request->send(LittleFS, "/setup_radio.html", String(), false, processor); //Χρήση processor αν στην HTML υπάρχει %ΛΕΞΗ%, τότε θα βάλει στη θέση ότι επιστρέψει η processor σαν ASP
               });
      //Ζητήθηκε η υποσελίδα /setup_net.html
      server.on("/setup_net.html", HTTP_GET, [](AsyncWebServerRequest * request)
               {
                if (!request->authenticate(LOG_NAME.c_str(), LOG_PASS.c_str()))
                    return request->requestAuthentication();
                else
                    request->send(LittleFS, "/setup_net.html", String(), false, processor_net); //Χρήση processor αν στην HTML υπάρχει %ΛΕΞΗ%, τότε θα βάλει στη θέση ότι επιστρέψει η processor σαν ASP
               });
      //Ζητήθηκε η υποσελίδα /log.html
      server.on("/log.html", HTTP_GET, [](AsyncWebServerRequest * request)
               {
                request->send(LittleFS, "/log.html", String(), false, processor); //Χρήση processor αν στην HTML υπάρχει %ΛΕΞΗ%, τότε θα βάλει στη θέση ότι επιστρέψει η processor σαν ASP
               });
      
      //Δεν βρέθηκε αρχείο
      server.onNotFound(notFound);

      //Ζητήθηκε το URL http://ip_address/status και απαντάει με το παρακάτω string σε μορφή text. Αυτό γίνεται κάθε 1sec και ενημερώνει τα στοιχεία span και div.
      server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request)
               {
                String tmp = "<val id=\"time\">" + curTime() + "</val>" + \
                             "<val id=\"up_time\">" + upTime() + "</val>" + \
                             "<val id=\"internet\">" + InternAvail + "</val>" + \
                             "<val id=\"srv_state\">" + NWSrv_State + "</val>" + \
                             "<val id=\"s_strength\">" + S_Strength + "</val>" + \
                             "<val id=\"rxpkt\">" + rx_pkts_str + "</val>" + \
                             "<val id=\"txpkt\">" + tx_pkts_str + "</val>" + \
                             "<val id=\"rxpktcnt\">" + String(rxpkt_cnt) + "</val>" + \
                             "<val id=\"txpktcnt\">" + String(txpkt_cnt) + "</val>" + \
                             "<val id=\"filename\">" + DLFileName + "</val>" + \
                             "<val id=\"Dl_OK\">" + Dl_OK + "</val>";
                request->send(200, "text/plain", tmp);
               });

      //Ζητήθηκε το αρχείο JQuery
      server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             request->send(LittleFS, "/jquery.min.js", "text/javascript");
            });

      //Ζητήθηκε το αρχείο Chart για δημιουργία γραφικών παραστάσεων
      server.on("/chart.min.js", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             request->send(LittleFS, "/chart.min.js", "text/javascript");
            });
  
      //Ζητήθηκε το αρχείο CSS
      server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             request->send(LittleFS, "/style.css", "text/css");
            });

      //========================== Ενέργειες με το πάτημα κουμπιών ========================================
      //Ζητήθηκε η υποσελίδα /update της αρχικής σελίδας με GET κάποιες παραμέτρους π.χ. /update?button1=Ok
      server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             //saveSettings(request, (char*)("/settings.txt"));
             //readSettings(); 
             //time2next = (int)(LoopTime * 60);
             /**
             if (request->hasParam("button1")) //Αν υπάρχει η παράμετρος
                {
                 Serial.println(request->getParam("button1")->value()); //Debug
                 //if (request->getParam("button1")->value() == "Ok")
                 //  Butn_Ok_Long_Click();
                } **/
             //Επέστρεψε πίσω την ιστοσελίδα
             request->send(LittleFS, "/index.html", String(), false, processor);
            });

      //Ζητήθηκε η υποσελίδα /save_wp με GET κάποιες παραμέτρους για να αποθηκεύει ρυθμίσεις της /setup_wp.html
      server.on("/save_lora", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             saveSettings(request, (char*)("/settings_lora.txt"));
             readSettings_LORA(); 
             initLoRa();
             //Επέστρεψε πίσω την ιστοσελίδα setup_wp.html
             request->send(LittleFS, "/setup_radio.html", String(), false, processor);
            });

      //Ζητήθηκε η υποσελίδα /save_net με GET κάποιες παραμέτρους για να αποθηκεύει ρυθμίσεις της /setup_wp.html
      server.on("/save_net", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             saveSettings(request, (char*)("/settings_net.txt"));
             readSettings_net();
             //Επέστρεψε πίσω την ιστοσελίδα setup_net.html
             request->send(LittleFS, "/setup_net.html", String(), false, processor_net);
             os_timer_setfn(&restartDelay, &rst, NULL);
             os_timer_arm(&restartDelay, 1000, false); //Μετά από 1sec κάνε reset ώστε να φορτώσει τις νέες ρυθμίσεις
            });

      //Ζητήθηκε η υποσελίδα /fw_update
      server.on("/fw_update", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             //Επέστρεψε πίσω την ιστοσελίδα fw_update.html
             request->send(LittleFS, "/fw_update.html", String(), false, processor_net);
             DLFileName = "";
             Dl_OK = "false";
             //Dl_State = 1; //Ξεκίνα διαδικασία download
            });

      //Ζητήθηκε η υποσελίδα /FlashFW της αρχικής σελίδας με GET κάποιες παραμέτρους π.χ. /FlashFW?flash=yes
      server.on("/FlashFW", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             if (request->hasParam("flash")) //Αν υπάρχει η παράμετρος
                {
                 String t = request->getParam("flash")->value();
                 if (t == "yes")
                    {
                     Serial.println("Flashing Files ..."); //Debug
                     //Dl_State = 5;
                    }
                 //Επέστρεψε πίσω την ιστοσελίδα
                 request->send(LittleFS, "/index.html", String(), false, processor_net);
                }        
            });

      //Ζητήθηκε η υποσελίδα /log της αρχικής σελίδας με GET κάποιες παραμέτρους π.χ. /log?Erase=yes
      server.on("/log", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             if (request->hasParam("Erase")) //Αν υπάρχει η παράμετρος
                {
                 String t = request->getParam("Erase")->value();
                 if (t == "yes")
                    {
                     //Serial.println("Erasing File ..."); //Debug
                     LittleFS.remove("/log.csv");
                    }
                 //Επέστρεψε πίσω την ιστοσελίδα
                 request->send(LittleFS, "/log.html", String(), false, processor);
                }
             else if (request->hasParam("View")) //Αν υπάρχει η παράμετρος save
                {
                 String t = request->getParam("View")->value();
                 if (t == "yes") //εμφάνισε το αρχείο σε άλλο tab
                    {
                     request->send(LittleFS, "/log.csv", "text/plain");
                    }
                 else if (t == "save") //Κατέβασε το αρχείο για αποθήκευση
                    {
                     request->send(LittleFS, "/log.csv", "text/csv");
                    }
                }        
            });

      //Ζητήθηκε η υποσελίδα /outlog της αρχικής σελίδας με GET κάποιες παραμέτρους π.χ. /outlog?Erase=yes
      //Εδώ αποθηκεύονται οι καταστάσεις των κουρτινών
      server.on("/outlog", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             if (request->hasParam("Erase")) //Αν υπάρχει η παράμετρος
                {
                 String t = request->getParam("Erase")->value();
                 if (t == "yes")
                    {
                     //Serial.println("Erasing File ..."); //Debug
                     LittleFS.remove("/output.csv");
                    }
                 //Επέστρεψε πίσω την ιστοσελίδα
                 request->send(LittleFS, "/log.html", String(), false, processor);
                }
             else if (request->hasParam("View")) //Αν υπάρχει η παράμετρος save
                {
                 String t = request->getParam("View")->value();
                 if (t == "yes") //εμφάνισε το αρχείο σε άλλο tab
                    {
                     request->send(LittleFS, "/output.csv", "text/plain");
                    }
                 else if (t == "save") //Κατέβασε το αρχείο για αποθήκευση
                    {
                     request->send(LittleFS, "/output.csv", "text/csv");
                    }
                }        
            });

      //Ζητήθηκε η υποσελίδα /chart της αρχικής σελίδας με GET κάποιες παραμέτρους π.χ. /log?Erase=yes
      /*server.on("/chart", HTTP_GET, [](AsyncWebServerRequest * request)
            {
             if (request->hasParam("Type")) //Αν υπάρχει η παράμετρος
                {
                 String t = request->getParam("Type")->value();
                 if (t == "irrad")
                    {
                     //Επέστρεψε πίσω την ιστοσελίδα
                     request->send(LittleFS, "/chart1.html", String(), false, processor);
                    }
                 else if (t == "extemp")
                    {
                     //Επέστρεψε πίσω την ιστοσελίδα
                     request->send(LittleFS, "/chart2.html", String(), false, processor);
                    }
                 else if (t == "intemp")
                    {
                     //Επέστρεψε πίσω την ιστοσελίδα
                     request->send(LittleFS, "/chart3.html", String(), false, processor);
                    }
                 else if (t == "humidi")
                    {
                     //Επέστρεψε πίσω την ιστοσελίδα
                     request->send(LittleFS, "/chart4.html", String(), false, processor);
                    }
                }       
            });*/

      // Εκκίνηση του Web Server
      server.begin();
     }

void startWiFi()
     {
      char hostname[15]; 
      //--- Στατική IP  ------------------------------------
      byte tmp[4];
      split(IP_ADDR, tmp, '.');
      IPAddress local_IP(tmp[0], tmp[1], tmp[2], tmp[3]);
      split(IP_GATE, tmp, '.');
      IPAddress gateway(tmp[0], tmp[1], tmp[2], tmp[3]);
      split(IP_MASK, tmp, '.');
      IPAddress subnet(tmp[0], tmp[1], tmp[2], tmp[3]);
      split(IP_DNS, tmp, '.');
      IPAddress primaryDNS(tmp[0], tmp[1], tmp[2], tmp[3]);   //optional
      WiFi.macAddress(MAC_array);
      MAC_char[18] = 0;
      sprintf(MAC_char,"%02x:%02x:%02x:%02x:%02x:%02x", MAC_array[0],MAC_array[1],MAC_array[2],MAC_array[3],MAC_array[4],MAC_array[5]);
      sprintf(hostname, "%s%02x%02x%02x", "lora_Gw-", MAC_array[3], MAC_array[4], MAC_array[5]);
      wifi_station_set_hostname(hostname); //Βάζει hostname αν έχει δυναμική απόδοση διεύθυνης
      Serial.println("MAC: " + String(MAC_char));   //", len=" + String(strlen(MAC_char)) );
      Serial.println("HOSTNAME: " + String(hostname));
      
      //--------------------------------------------------------------------
      byte retry_cnt = 0;
      if (SetSTA == "true")
         {
          WiFi.mode(WIFI_STA);
          if (IP_STATIC == "true")
             {
              //Αν έχει στατική IP βγάζω το σχόλιο κάτω
              WiFi.config(local_IP, gateway, subnet, primaryDNS);
             }
          //--- Λειτουργία client ----------------------------------------------
          char client_ssid[20];
          ST_SSID.toCharArray(client_ssid, 20);
          char client_password[20];
          ST_KEY.toCharArray(client_password, 20);
          WiFi.begin(client_ssid, client_password);
          //Περίμενε 30sec ή μέχρι να γίνει σύνδεση
          while (WiFi.status() != WL_CONNECTED && retry_cnt < 30) 
                {
                 delay(1000);
                 Serial.println("Connecting to WiFi..");
                 retry_cnt++;
                }
          if (WiFi.status() == WL_CONNECTED)  //Αν έγινε σύνδεση
             {
              Serial.println("Start as Client connected to SSID: " + ST_SSID);
              Serial.print(F("Connect with your browser to "));
              Serial.println(WiFi.localIP());
             }
          else //Αλλιώς δεν έγινε σύνδεση
             {
              Serial.print(F("Unable to connect as Client. Please check SSID and Password"));
             }
         }
      else if (SetAP == "true")
         {
          //--- Λειτουργία AP.----------------------------------------------------
          char ap_ssid[20];
          AP_SSID.toCharArray(ap_ssid, 20);
          char ap_password[20];
          AP_KEY.toCharArray(ap_password, 20);
          WiFi.mode(WIFI_AP);
          int chan = AP_CHANNEL.toInt();
          WiFi.softAP(ap_ssid, ap_password, chan, 0, 4); //int channel, int ssid_hidden [0,1], int max_connection [1-4]
          //Δεν λειτουργεί τόσο σωστά με στατική IP. Καλύτερα να αφήνουμε την δυναμική δηλ. 192.168.4.1
          if (IP_STATIC == "true")
              WiFi.softAPConfig (local_IP, gateway, subnet);
          IPAddress IP = WiFi.softAPIP();
          Serial.println("Start as AP with SSID: " + AP_SSID);
          Serial.print(F("Connect with your browser to "));
          Serial.println(IP);
         }
     }

//Αν δεν υπάρχει κάποιο αρχείο στο σύστημα αρχείων τότε να εμφανίσει 404
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/html", "<!DOCTYPE html><HTML><HEAD><TITLE>CLIMA CONTROL : File Not Found</TITLE></HEAD><BODY><H1>Not found</H1></BODY></HTML>");
}

//Ψάχνει για placeholders μέσα στο αρχείο HTML με σύνταξη %xxx%, όπως η ASP
String processor(const String& var)
{
  //Serial.println(var); //Debug
  //Σελίδα index.html
  if (var == "VERSION") //Αν το βρήκες
      return String(VERSION); //Γύρνα μια τιμή
  else if (var == "RXPKTS_STR")
      return rx_pkts_str;
  else if (var == "TXPKTS_STR")
      return tx_pkts_str;
  else if (var == "RXPKTCNT")
      return String(rxpkt_cnt);
  else if (var == "TXPKTCNT")
      return String(txpkt_cnt);
  else if (var == "CTIME")
      return curTime();
  else if (var == "SSTRENGTH")
      return S_Strength;
  else if (var == "RADIOSTATUS")
      return "Freq:" + lora_freq + "Hz SF:" + lora_sf + " BW:" + lora_bw + "Hz CR: 4/" + lora_cr;
  //Σελίδα setup_radio.html
  else if (var == "FREQ")
      return lora_freq;
  else if (var == "SF")
      return lora_sf;
  else if (var == "CR")
      return lora_cr;
  else if (var == "BW")
      return lora_bw;
  else if (var == "SYNC")
      return String(lora_sync.toInt(), HEX);
  else if (var == "POWER")
      return lora_power;
  
  //Σελίδα chartXX.html
  //else if (var == "ChartDataLabl")
  //    return ChartData(0, true);
  //else if (var == "ChartData5")
  //    return ChartData(5, false);
  //else if (var == "ChartData1")
  //    return ChartData(1, false);
  //else if (var == "ChartData2")
  //    return ChartData(2, false);
  //else if (var == "ChartData3")
  //    return ChartData(3, false);
  return String(); //Αν δεν το βρήκες γύρνα κενό string
}

//Ψάχνει για placeholders μέσα στο αρχείο HTML με σύνταξη %xxx%, όπως η ASP
String processor_net(const String& var)
{
  //Serial.println(var); //Debug
  //Σελίδα setup_net.html
  if (var == "VERSION") //Αν το βρήκες
      return String(VERSION); //Γύρνα μια τιμή
  else if (var == "SetAP")
      return SetAP;
  else if (var == "AP_SSID")
      return AP_SSID;
  else if (var == "AP_CHANNEL")
      return AP_CHANNEL;
  else if (var == "AP_ENCRYPT")
      return AP_ENCRYPT;
  else if (var == "AP_KEY")
      return AP_KEY;
  else if (var == "SetSTA")
      return SetSTA;
  else if (var == "ST_SSID")
      return ST_SSID;
  else if (var == "ST_KEY")
      return ST_KEY;
  else if (var == "IP_STATIC")
      return IP_STATIC;
  else if (var == "IP_ADDR")
      return IP_ADDR;
  else if (var == "IP_MASK")
      return IP_MASK;
  else if (var == "IP_GATE")
      return IP_GATE;
  else if (var == "IP_DNS")
      return IP_DNS;
  else if (var == "NTP_TZ")
      return NTP_TZ;
  else if (var == "BROW_TO")
      return BROW_TO;
  else if (var == "NTP_SRV")
      return NTP_SRV;
  else if (var == "NW_SRV")
      return NET_SRV;
  else if (var == "NW_PORT")
      return NET_SRV_PORT;
  else if (var == "NW_USER")
      return MQTT_USER;
  else if (var == "NW_PASS")
      return MQTT_PASS;
  else if (var == "NW_CLIENT")
      return MQTT_CLIENT;
  else if (var == "LOG_NAME")
      return LOG_NAME;
  else if (var == "LOG_PASS")
      return LOG_PASS;
  return String(); //Αν δεν το βρήκες γύρνα κενό string
}

//Αποθηκεύει ρυθμίσεις της αρχικής οθόνης που είναι σε μορφή GET
void saveSettings(AsyncWebServerRequest *request, char *Filename)
     {
      File file = LittleFS.open(Filename, "w");
      if (!file) 
         {
          Serial.println("Error opening file for writing");
          return;
         }
      file.print("#Automatic created from script\n");
      int args = request->args();
      for (int i = 0; i < args; i++)
          {
           file.print(String(request->argName(i).c_str()) + "::" + String(request->arg(i).c_str()) + "\n");
          }
      file.close();
     }

//Έλεγχος σύνδεσης στο διαδίκτυο
//Κάθε 30 sec στείλε αίτημα σύνδεσης 
static void testcon(void* arg)
{
 test_client->connect(testhost, 80); //Στείλε
 if (conn) //Αν το conn έγινε true μέσα στην onConnect υπάρχει σύνδεση
    {
     conn = false; //Άλλαξέ το για την επόμενη προσπάθεια
     InternAvail = "true"; //Θέσε μεταβλητή που στέλνει στο web interface
     //Serial.println("Internet OK");
    }
 else //Αλλιώς δεν υπάρχει σύνδεση
    {
     InternAvail = "false";
     //Serial.println("Internet BAD");
    }
 //Κάνει έλεγχο για τον τρόπο αναβοσβήματος του status LED (μπλέ)
 if (SetSTA == "true") //Αν συνδέεται σαν client
    {
     if (WiFi.status() == WL_CONNECTED  && InternAvail == "true" && NWSrv_State == "true")
         led_pat = 0; //Αναμμένο συνέχεια
     else if (WiFi.status() == WL_CONNECTED && InternAvail == "true" && NWSrv_State == "false")
         led_pat = 0b0000011111; //Duty cycle 50%
     else if (WiFi.status() == WL_CONNECTED && InternAvail == "false" && NWSrv_State == "false") 
         led_pat = 0b0101111111; //Δύο σύντομες αναλαμπές
     else if (WiFi.status() != WL_CONNECTED)
        {
         led_pat = 0b0111111111; //Μία σύντομη αναλαμπή
         char client_ssid[20];
         ST_SSID.toCharArray(client_ssid, 20);
         char client_password[20];
         ST_KEY.toCharArray(client_password, 20);
         WiFi.begin(client_ssid, client_password);
         Serial.println("Trying to reconnected as client to SSID: " + ST_SSID);
        }
    }
 else if (SetAP == "true") //Αν λειτουργεί σαν AP
     led_pat = 0b0000100001; //Δύο σύντομες διακοπές
}

//Αν έγινε σύνδεση με τον testhost
void onConnect(void* arg, AsyncClient* client) 
{
  //Serial.println("client has been connected");
  if (client->connected()) //Αν είναι συνδεμένο
     {
      conn = true; //Θέσε το conn ώστε να ανιχνευθεί στην testcon κατά την επόμενη προσπάθεια
      client->stop(); //Αποσύνδεση
     }
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
char* iso_time()
     {
      unsigned long dd = timeClient.getUTCEpochMillis() - timeClient.getUTCEpochTime() * 1000; //Υπολόγισε τα millis
      sprintf(isotime, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldz", timeClient.getUTCYear(), timeClient.getUTCMonth(), timeClient.getUTCDay(), 
                       timeClient.getUTCHours(),timeClient.getUTCMinutes(), timeClient.getUTCSeconds(), dd);
      return isotime;
     }

byte* header(byte pkgType, short r1 = 0, short r2 = 0)
     {
      if (r1 < 0) r1 = random(256);
      if (r2 < 0) r2 = random(256);
      head[0] = 0x01; head[1] = r1; head[2] = r2; head[3] = pkgType; 
      head[4] = MAC_array[0]; head[5] = MAC_array[1]; head[6] = MAC_array[2];
      head[7] = 0xff; head[8] = 0xff;
      head[9] = MAC_array[3]; head[10] = MAC_array[4]; head[11] = MAC_array[5];
      return head;
     }

void sdpkt(byte *p, unsigned short len)
     {
      UDP.begin(NET_SRV_PORT.toInt());
      UDP.beginPacket(NET_SRV.c_str(), NET_SRV_PORT.toInt());
      UDP.write((char*)p, len);
      UDP.endPacket();
      Serial.print(F("Send to NW Server: "));
      //print_table(p, len, 'h'); //Με 'c' εμφανίζει ASCII για να ελέγχω το JSON
     }

void pull_data()
     {
      sdpkt(header(0x02, -1, -1), 12);
     }

void tx_ack()
     {
      head[0] = 0x01; head[1] = 0; head[2] = 0; head[3] = 5;
      memcpy(buff_up, head, 4);
      buff_index = 4;
      memcpy((buff_up + buff_index), "\0", 1); 
      buff_index += 1;
      buff_up[buff_index] = 0;
      sdpkt(buff_up, buff_index);
     }

void push_data()
     {
      header(0x00, -1, -1); //Ετοιμάζει το header
      last_h1 = head[1]; last_h2 = head[2];
      memcpy(buff_up, head, 12);
      buff_index = 12;
      memcpy((buff_up + buff_index), "{\"rxpk\": [{", 11); 
      buff_index += 11;
      //Ετοιμάζει το πακέτο για αποστολή στον NW Server
      float freq = (frequency / 1000000.0);
      buff_index += snprintf((char *)(buff_up + buff_index), 1024-buff_index,
                    "\"time\":\"%s\",\"rfch\":%1u,\"freq\":%.3f,\"stat\":1", 
                           iso_time(),           0,         freq);

      buff_index += snprintf((char *)(buff_up + buff_index), 1024-buff_index, 
                    ",\"datr\":\"SF%uBW%u\",\"codr\":\"4/5\",\"lsnr\":%li,\"rssi\":%d,\"size\":%u,\"data\":\"",
                   (unsigned int)lora_sf.toInt(), (unsigned int)(lora_bw.toInt()/1000),    (long)snr,     rssi,      msgLen);
      
      buff_index += encode_base64(incoming, msgLen, (byte*)(buff_up + buff_index)); //Σε περίπτωση που θέλουμε κωδικοποίηση Base64
  
      buff_index += snprintf((char *)(buff_up + buff_index), 1024-buff_index, 
                    "\",\"tmst\":%u", 
                    (uint32_t)(tx_time));  //+RX1_DELAY?
      
      buff_up[buff_index]   = '}';
      buff_up[buff_index+1] = ']';            // According to specs, this] can remove
      buff_up[buff_index+2] = '}'; 
      buff_index += 3;
      //Serial.println(buff_index); //Debug
      buff_up[buff_index] = 0;
      sdpkt(buff_up, buff_index);
      //tx_tmst = 4294967295;
      //tx_wait = true;
     }

String parse_json(String s, String key)
{
 String key1 = "\"" + key + "\":"; //Φτιάξε το key π.χ. "data":
 String tmp1, tmp2 = "";
 short idx_s = s.indexOf(key1); //Ψάξε για το κλειδί
 short idx_e = -1;
 if (idx_s > 0) //Αν το βρήκε
    {
     tmp1 = s.substring(idx_s + key1.length()); //Πάρε από εκεί και μετά
     if (tmp1.indexOf("}") > 0) //Αν υπάρχει } ή , τότε τελειώνει το πεδίο json
         idx_e = tmp1.indexOf("}");
     if (tmp1.indexOf(",") > 0)
         idx_e = tmp1.indexOf(",");
     if (idx_e > 0) //Αν τελειώνει
        {
         tmp2 = tmp1.substring(0, idx_e); //Πάρε από την αρχή μέχρι το , ή }
         tmp2.trim(); //Αφαίρεση κενά μπρος και πίσω
         if (tmp2[0] == '"' and tmp2[tmp2.length() - 1] == '"') //Αν έχει στην αρχή και το τέλος " τότε αφαίρεσε τα
             tmp2 = tmp2.substring(1, tmp2.length() - 1);
        }
    }
 return tmp2;
}
