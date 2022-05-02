#define VERSION       "1.01"

//#define LED1                  2 //Build in Led  
#define BUILTIN_LED           2
#define PULLDATA_INTERVAL     30000  //Κάθε 30sec στέλνει pull data στον server
#define PRE_OPEN_TIME         2000

byte head[12];  //Η επικεφαλίδα του πακέτου UDP
char isotime[25]; //Ο χρόνος λήψης σε μορφή ISO
unsigned long pull_timer; //Ο χρονιστής για Pull Data
byte last_h1, last_h2;
unsigned long blink_per; //Ο χρονιστής για το LED
String ipol;
unsigned long tx_tmst;
byte sf, size;
unsigned int bw;
bool tx_wait;

#define BUFF_LEN     5   //Πόσα πακέτα από τον NW Server αποθηκεύει μέχρι να μεταδοθούν από το LoRa
struct pkt{ byte txbuf[256]; //Μέγιστο μήκος κάθε πακέτου 256 bytes
            int frame_len;
            byte sf;
            long bw;
            unsigned long tx_tmst;
            String ipol;
          };

pkt packets[BUFF_LEN];

//--------------------- Όνομα του Server για να κάνει έλεγχο σύνδεσης ------------------------------------
const char *testhost = "www.google.com";  //Η διεύθυνση για να κάνει έλεγχο σύνδεσης στο διαδίκτυο
const char *update_host = "hostname";
String Dl_Path = "/path/";
String Dl_Filename; //Η πλήρης διαδρομή του αρχείου π.χ. /hostname/path/test.txt δημιουργείται μέσα στην download() στο network.h
String FileName, Version; //Το όνομα του αρχείου στο SPIFFS
byte Dl_State = 0;
bool CanUpdate;

unsigned long tim; //Χρονιστής epoch αν δεν έχει σύνδεση διαδικτύου

//-------------------------- Μεταβλητές σύνδεσης μέσω WiFi -----------------------------------------------
String SetAP, AP_SSID, AP_CHANNEL, AP_ENCRYPT, AP_KEY, SetSTA, ST_SSID, ST_KEY, IP_STATIC, IP_ADDR, IP_MASK,
       IP_GATE, IP_DNS, NTP_TZ, BROW_TO, NTP_SRV, NET_SRV, NET_SRV_PORT, MQTT_USER, MQTT_PASS, MQTT_CLIENT, LOG_NAME="admin", LOG_PASS="preveza";
String InternAvail = "false", NWSrv_State = "false", DLFileName = "", Dl_OK = "false", S_Strength = "";

bool conn = false; //Γίνεται true όταν υπάρχει σύνδεση Internet

#define LOG_LINES   10
char rx_pkt_log[60], tx_pkt_log[60];
String rx_pkts_log[LOG_LINES], rx_pkts_str, tx_pkts_log[LOG_LINES], tx_pkts_str;
unsigned int rxpkt_cnt = 0, txpkt_cnt = 0;

//---------------------------------------------------------------------
unsigned long uptime; //Χρονιστής uptime για έλεγχο συνεχόμενης λειτουργίας

//----------------------------- Μεταβλητές LORA -----------------------------------------
#define FREQ_START            433050000
#define FREQ_END              434790000   //Εύρος 1.74MHz δηλαδή 28 κανάλια 62.5KHz
#define FREQUENCY             433150000
#define SF                            9
#define BW                       125000     //Hz
#define CR                            5
#define SYNCWORD                    194     //0xC2 ή 0x34 για LORAWAN
#define POWER                        17     //dBm
String lora_freq, lora_sf, lora_cr, lora_bw, lora_sync, lora_power; 

char t_ext_str[6], t_int_str[6];
short cur_Irrad, cur_Irrad_avg = 0, max_Irrad = -50;
char cur_Irrad_str[6];
float t_ext, t_int, max_t_ext = -40.0, max_t_int = -40.0, min_t_ext = 100.0, min_t_int = 100.0;
float dht_temp = 0.0;
char dht_temp_str[6];
float humidity = 0.0, humWet = 0.0, max_humidity = -10.0, min_humidity = 200.0;
char hum_str[6]; //Εδώ αποθηκεύει την τιμή της υγρασίας
char wet_str[6];
char max_t_ext_timestamp[35], min_t_ext_timestamp[35], max_t_int_timestamp[35], min_t_int_timestamp[35], max_Irrad_timestamp[42], max_Humidity_timestamp[35], min_Humidity_timestamp[35];
float T_int_S = 0, T_ext_S = 0, dht_temp_S = 0, humidity_S = 0; //Αθροίσματα τιμών κάθε 1sec
float T_int_SHr = 0, T_ext_SHr = 0, humidity_SHr = 0, dht_temp_SHr = 0; //Αθροίσματα τιμών κάθε 5min
//-----------------------------------------------------------------------------

unsigned short led_pat = 0;
