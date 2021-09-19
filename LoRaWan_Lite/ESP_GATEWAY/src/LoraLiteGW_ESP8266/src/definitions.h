#define LED1                  2 //Build in Led  
#define PULLDATA_INTERVAL     30000  //Κάθε 30sec στέλνει pull data στον server
#define PRE_OPEN_TIME         2000

enum {
      F1 = 433175000,
      F2 = 433275000,
      F3 = 433475000,
      F4 = 433675000,
      F5 = 433875000,
      F6 = 434074000,
      F7 = 434275000,
      F8 = 434475000
     };
byte head[12];  //Η επικεφαλίδα του πακέτου UDP
char isotime[25]; //Ο χρόνος λήψης σε μορφή ISO
unsigned long pull_timer; //Ο χρονιστής για Pull Data
byte last_h1, last_h2;
unsigned long blink_per; //Ο χρονιστής για το LED
String ipol;
unsigned long tx_tmst;
byte sf, size;
bool tx_wait;

#define BUFF_LEN     5   //Πόσα πακέτα από τον NW Server αποθηκεύει μέχρι να μεταδοθούν από το LoRa
struct pkt{ byte txbuf[256]; //Μέγιστο μήκος κάθε πακέτου 256 bytes
            int frame_len;
            byte sf;
            unsigned long tx_tmst;
            String ipol;
          };

pkt packets[BUFF_LEN];