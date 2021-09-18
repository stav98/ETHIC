void enterSleep(void);
//void print_table(byte*, byte, char);
//void blink_err_code(void);
/*
  LoRa Duplex communication wth callback

  Sends a message every half second, and uses callback
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.
*/
#include "lora_modem.h"

#define CS_PIN                10  //LoRa radio Chip Select (CS) pin
#define RESET_PIN             9   //LoRa radio Reset pin
#define IRQ_PIN0              2   //Το interrupt Pin πρέπει να είναι Interrupt Pin
#define IRQ_PIN1              3   //Το interrupt Pin πρέπει να είναι Interrupt Pin

//DR 5
#define SF            7  //Κανονικά είναι 7
#define BW            125000     //Hz
//DR 0
//#define SF            12
//#define BW            62500     //Hz 125000
#define CR            5          //4/5
#define MAX_TX_PACKT  (MSG_LEN + 16)       //220 default - MSG_LEN + 16 γιατί 16 bytes είναι τα headers
#define MAX_RX_PACKT  (MSG_LEN + 16)       //220 default - MSG_LEN + 16 γιατί 16 bytes είναι τα headers

#define WAIT_FOR_RX1    0
#define RX1_WIND_OPEN   1
#define RX2_WIND_OPEN   2
#define RX_DONE        3

unsigned int ChkSum(byte*, byte, byte);

byte tx_buf[MAX_TX_PACKT];   //Buffer για δεδομένα εκπομπής LoRa
byte rx_buf[MAX_RX_PACKT];   //Buffer για δεδομένα λήψης LoRa
void sendMessage(String);
boolean reply_en = false;
void onReceive(int);
void onRxTimeOut(void);

void lora_init()
{
  //=== Αρχικοποίηση πομποδέκτη LORA ===
  LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN0, IRQ_PIN1); // Όρισε τα pins CS, RESET, IRQ0 και IRQ1
  if (!LoRa.begin(FREQUENCY))   //Αρχικοποίησε τον πομποδέκτη στη συχνότητα που βρήκες στην EEPROM
     {             
      Serial.println(F("LoRa init failed."));
      while (true)  //Αν δεν ξεκίνησε κόλλα εκεί
            {
             error_code = SETUP_ERROR;
             blink_err_code();
            }
     }
  LoRa.onReceive(onReceive);              //Βάλε ISR για λήψη
  LoRa.onRxTimeOut(onRxTimeOut);          //Βάλε ISR για Rx Timout
  LoRa.setSignalBandwidth(BW);            //Το Bandwidth στα 62500 Hz. Ταχύτητα περίπου 2700 bps με ευαισθησία -124dBm
  LoRa.setSpreadingFactor(SF);
  LoRa.setCodingRate4(CR);                 //4/5
  LoRa.setTxPower(17);                    //Ισχύς εξόδου +17dBm, Εξ' ορισμού λειτουργεί ο PA με ισχύ από +2 έως +17dBm
  LoRa.setSyncWord(0xc2);                 //το LoRa Wan έχει 0x34 εγώ βάζω C2
  LoRa.enableCrc();
  //Serial.println(LoRa.getSyncWord(), HEX);
  //LoRa.disableCrc();
  LoRa.setRxSymbTimeout(6);       //<--10 //Αυτό default είναι 100 με 10 * Ts (1.24ms για SF7 - 125Khz) ανοίγει παράθυρο ~10msec
  LoRa.disableInvertIQ();                 //I/Q κανονικά για uplink προς πύλη
  LoRa.idle();                            //Πέσε σε ανενεργή κατάσταση
  Serial.println(F("LoRa init OK!\n"));
}

//Αν υπάρχουν δεδομένα. Λειτουργία interrupt ISR
void onReceive(int packetSize) 
{
  unsigned int dest_addr, chks, FCntDown;
  int i;
  if (packetSize == 0) return;          //Αν δεν υπάρχει πακέτο φύγε από εδώ
  //Όταν χρησιμοποιώ callback όπως τώρα, δεν μπορώ να χρησιμοποιήσω καμία συνάρτηση της Stream όπως
  //readString, parseInt() κλπ. Επομένως
  for (i = 0; i < packetSize; ++i)
       rx_buf[i] = LoRa.read();
  rx_state = RX_DONE;
  if (rx_buf[0] & 64) //Αν είναι μήνυμα από πύλη δηλ. Downlink
  {
   Serial.println(F("Rx Frame <"));
   print_table(rx_buf, i, 'h'); //Εμφάνιση κωδικοποιημένων δεδομένων λήψης, το i έχει το μήκος του πακέτου λήψης
   dest_addr =  ((unsigned int) rx_buf[2] * 256) + (unsigned int) rx_buf[1]; //Υπολογισμός διεύθυνσης παραλήπτη
   FCntDown = ((unsigned int) rx_buf[4] * 256) + (unsigned int) rx_buf[3]; //Μετρητής Frame λήψης
   //Το Checksum γίνεται XOR με την διεύθυνση
   chks = ((rx_buf[i-2] ^ rx_buf[1]) + ((rx_buf[i-1] ^ rx_buf[2]) << 8)); //Διάβασε το Check Sum από το τέλος του frame
   //Υπολογισμός CheckSum
   unsigned int s = ChkSum(rx_buf, 0, i - 2);
   if (dest_addr == NODE_ADDRESS) //Αν με αφορά
      {
       Serial.println(F("--- Message for me! ---"));
       rssi = LoRa.packetRssi();
       Serial.println("  RSSI: " + String(rssi));
       Serial.println("   Snr: " + String(LoRa.packetSnr()));
       //Αν το frame έφτασε σωστά
       if (s == chks)
          {
           if (rx_buf[0] & 128) //Αν από NW ζητήθηκε επιβεβαίωση λήψης τότε ετοίμασε ACK για επόμενο Uplink
               confirmed = true;
           else //Διαφορετικά ήταν σκέτο ACK από το NW και δεν θα θέσει το ACK στο επόμενο Uplink
               confirmed = false;
           if (rx_buf[0] & 16)
              {
               Serial.println(F("Ack received from NW Server"));
               Ack_OK = true;
              }
           if (i > (8 + (rx_buf[0] & 7))) //Αν το μήκος χωρίς Fopts είναι μεγαλύτερο από 8 τότε υπάρχει μήνυμα διαφορετικά είναι μόνο ACK 
              {
               decrypt(rx_buf, rx_message, i - 2); //το i έχει το μήκος του πακέτου λήψης. Τα 2 τελευταία είναι CheckSum
               Serial.print(F("Message: "));
               Serial.println(FCntDown);
               Serial.println(rx_message);
               if (String(rx_message) == "LED1=On")
                  digitalWrite(LED1, HIGH);
               else if (String(rx_message) == "LED1=Off")
                  digitalWrite(LED1, LOW);
              }
          }
      else
          Serial.println(F("*** CheckSum Error ***"));
     }
  }
  cont = 2;
}

//Αν έγινε timout λήψης χωρίς να πάρει έγκυρη απάντηση
void onRxTimeOut()
{
 unsigned long d;
 d = micros()- rx_time;
 Serial.print(F("Timeout after: "));
 Serial.println(d);
 cont++;
}

//Προετοιμάζει το πακέτο LoRa Lite για εκπομπή
//____1_______2_______2______0-15_____1_________1-247__________2______
//| TYPE | ADDRESS | FCNT | FOPTS | FPORT |      DATA     | CHECKSUM |
//|______|_________|______|_______|_______|_______________|__________|
//TYPE:b7,b6 = MTYPE : 00 Unconf Data Up, 01 Unconf Data Down, 10 Confirmed Data Up, 11 Conf Data Down
// -- :b5 = ADR
// -- :b4 = ACK
// -- :b3 = F-PENDING
// -- :b0-b2 = OPTIONS LENGTH 
byte prep_pkt(byte Type, unsigned int Address, unsigned int Fcnt, byte Fport, char* Data, byte n)
{
 byte i;
 for (i = 0; i < MAX_TX_PACKT; ++i)
      tx_buf[i] = 0;
 tx_buf[0] = Type; tx_buf[1] = Address; tx_buf[2] = (Address >> 8); tx_buf[3] = Fcnt; tx_buf[4] = (Fcnt >> 8); tx_buf[5] = Fport;
 for (i=0; i < n; i++)
     {
      tx_buf[6 + i] = Data[i]; //Αντιγραφή δεδομένων στο tx_buf
     }
 unsigned int s = ChkSum(tx_buf, 0, i + 6);
 tx_buf[i+6] = s ^ tx_buf[1]; 
 ++i;
 tx_buf[i+6] = (s >> 8) ^ tx_buf[2];
 ++i;
 return (i+6); //Επιστροφή συνολικού μήκους
}

//Υπολογισμός CheckSum
unsigned int ChkSum(byte* data, byte start, byte end)
{
 byte i;
 unsigned int s = 0;
 unsigned long t = 0;
 for (i = 0; i < end; i += 2) //Για την επικεφαλίδα και όλα τα δεδομένα
     {
      s += ((unsigned int) (data[i] << 8) + data[i+1]); 
      t += ((unsigned int) (data[i] << 8) + data[i+1]); 
      if (t > 0xffff)
         {
          t = s;
          s += 1;  
         }
     }
 return s;
}

void send_pkt(char* payload, byte n)
{
 byte l, Type;
 //Αν το μήνυμα που έλαβε πριν από τον NW server έχει πληροφορία και ζητάει επιβεβαίωση, τότε να θέσει το ACK
 if (confirmed)
     Type = CONFIRM_REQ | 16; //Βάλε το bit ACK = 1
 else //Διαφορετικά ήταν μόνο ACK και δεν θέλει επιβεβαίωση
     Type = CONFIRM_REQ;
 l = prep_pkt(Type, NodeAddress, Fcnt, 0x01, out, n); //Ετοίμασε πακέτο με header 00 ή 80 (χωρίς επιβ. - με επιβ.), διεύθυνση, Α/Α, πόρτα, δεδομένα - επιστρέφει μήκος
 print_table(tx_buf, l, 'h'); //Debug
 LoRa.disableInvertIQ();      //Τα I/Q κανονικά για εκπομπή
 LoRa.beginPacket();          //Ξεκίνα πακέτο
 LoRa.write(tx_buf, l);       //Αντιγραφή δεδομένων μήκους l στο FIFO
 LoRa.endPacket();            // Κλείσε το πακέτο και στείλτο
 rx_time = micros();          //Κράτα τον χρόνο ώστε σε λίγο (1sec) να ανοίξει το παράθυρο λήψης
 LoRa.idle();                 //Επιστροφή σε κατάσταση Idle
 rx_state = WAIT_FOR_RX1;
}