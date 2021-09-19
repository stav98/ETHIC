#include <NTPClient_Generic.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>

#define TIME_ZONE_OFFSET_HRS          (+3)
#define UDP_PORT                      1700
#define SERVER_IP                     "192.168.42.33"
//#define SERVER_IP                     "example.com"
byte buff_down[512]; 
byte buff_up[1024];  
unsigned short buff_index = 0;

//--------------- Λειτουργία client --------------
char client_ssid[] = "WiFi_SSID";
char client_password[] = "WiFi_Password";

//--------------- Λειτουργία AP ------------------
//const char* ap_ssid     = "ESP32-Access-Point";
//const char* ap_password = "123456789";

char reply[] = "Packet received!";

uint8_t MAC_array[6];
char MAC_char[19];     

//Ορισμός NTP Client για συγχρονισμό της ώρας
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "gr.pool.ntp.org", TIME_ZONE_OFFSET_HRS * 3600);

//Ορισμός αντικειμένου UDP
WiFiUDP UDP;

void initNetwork()
     {
      char hostname[15];
      //--- Στατική IP μόνο για clients ------------------------------------
      IPAddress local_IP(192, 168, 42, 123);
      IPAddress gateway(192, 168, 42, 5);
      IPAddress subnet(255, 255, 255, 0);
      IPAddress primaryDNS(192, 168, 42, 5);   //optional
     
      //--------------------------------------------------------------------
      WiFi.mode(WIFI_STA);
      //Αν έχει στατική IP βγάζω το σχόλιο κάτω
      //WiFi.config(local_IP, gateway, subnet, primaryDNS);   
      WiFi.macAddress(MAC_array);
      MAC_char[18] = 0;
      sprintf(MAC_char,"%02x:%02x:%02x:%02x:%02x:%02x", MAC_array[0],MAC_array[1],MAC_array[2],MAC_array[3],MAC_array[4],MAC_array[5]);
      
      sprintf(hostname, "%s%02x%02x%02x", "lora_gw-", MAC_array[3], MAC_array[4], MAC_array[5]);
      wifi_station_set_hostname(hostname); //Βάζει hostname αν έχει δυναμική απόδοση διεύθυνης
      Serial.println("MAC: " + String(MAC_char));   //", len=" + String(strlen(MAC_char)) );
      Serial.println("HOSTNAME: " + String(hostname));
      //--- Λειτουργία client ----------------------------------------------
      WiFi.begin(client_ssid, client_password);
      while (WiFi.status() != WL_CONNECTED) 
            {
             delay(1000);
             Serial.println("Connecting to WiFi..");
            }
       //
      Serial.println(WiFi.localIP());
      /*
      //--- Λειτουργία AP. Διεύθυνση 192.168.4.1 --------------------------- 
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ap_ssid, ap_password, 1, 0, 2); //int channel, int ssid_hidden [0,1], int max_connection [1-4]
      //IPAddress IP = WiFi.softAPIP();
      //Serial.print("AP IP address: ");
      //Serial.println(IP);*/

      //UDP.begin(UDP_PORT);
      //UDP.beginPacket(SERVER_IP, UDP_PORT);
      //UDP.write(reply);
      //UDP.endPacket();
    
      //Ξεκίνησε τον NTP Client
      timeClient.begin();
      timeClient.setUpdateInterval(SECS_IN_HR); //Αν καλείται στο Loop θα κάνει update μια φορά την ώρα
      bool ntp_OK = false;
      byte ntp_tries = 0;
      while (!ntp_OK && ntp_tries < 10)
            {
             timeClient.update();
             if (timeClient.updated())
                 ntp_OK = true;
             ntp_tries++;
             delay(100);
            }
      if (ntp_OK)
          Serial.println(F("Time Client Updated"));
      else
          Serial.println(F("Time Client Not Updated"));
      Serial.println("UTC : " + timeClient.getFormattedUTCDateTime());
      //Serial.println("LOC : " + timeClient.getFormattedDateTime());
     }

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
      UDP.begin(UDP_PORT);
      UDP.beginPacket(SERVER_IP, UDP_PORT);
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
                    "\"time\":\"%s\",\"chan\":%1u,\"rfch\":%1u,\"freq\":%.3f,\"stat\":1", 
                           iso_time(),         1,           0,         freq);

      buff_index += snprintf((char *)(buff_up + buff_index), 1024-buff_index, 
                    ",\"datr\":\"SF%uBW%u\",\"codr\":\"4/5\",\"lsnr\":%li,\"rssi\":%d,\"size\":%u,\"data\":\"",
                                   SF, (BW/1000),                  (long)snr,     rssi,      msgLen);
      
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
