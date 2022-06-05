//Με κεραία πηνίο στο Node στο σπίτι του πατέρα μου και GP λ/4 στο σπίτι μου έχω -100dbm. Αν στο node βάλω σύρμα λ/4 έχω -95dbm
//Για SF7-125Khz λειτουργεί και αποκωδικοποεί μέχρι -117dbm
#include "lora_modem.h"

#define CS_PIN                10  //LoRa radio Chip Select (CS) pin
#define RESET_PIN             -1  //LoRa radio Reset pin
#define IRQ_PIN0               2  //Το interrupt Pin πρέπει να είναι Interrupt Pin
#define IRQ_PIN1               3  //Το interrupt Pin πρέπει να είναι Interrupt Pin


//Με SF9-125KHz με μήκος πακέτου 64byte που είναι το δικό μου έχει χρόνο εκπομπής 370msec για κάθε ένα λεπτό 60*24*370 = 532sec ή 0,6%
//Με SF9-62.5KHz με μήκος πακέτου 64byte που είναι το δικό μου έχει χρόνο εκπομπής 740msec για κάθε ένα λεπτό 60*24*740 = 1066sec ή 1,2%
#define SF             7          //Κανονικά είναι 7. Υποστηρίζει 7, 8, 9, 10, 11, 12. Το παράθυρο Rx2 είναι 12
#define BW             125000     //Hz. Λειτουργεί καλά με 62.5, 125, 250 KHz
#define CR             5          //4/5. Υποστηρίζει 5, 6, 7, 8 δηλ, 4/5, 4/6, 4/7, 4/8
#define POWER          20         //dBm. Από 2 - 20
#define MAX_TX_PACKT  (MSG_LEN + 16)       //220 default - MSG_LEN + 16 γιατί 16 bytes είναι τα headers
#define MAX_RX_PACKT  (MSG_LEN + 16)       //220 default - MSG_LEN + 16 γιατί 16 bytes είναι τα headers

//Καταστάσεις της μεταβλητής rx_state
#define WAIT_FOR_RX1    0
#define RX1_WIND_OPEN   1
#define RX2_WIND_OPEN   2
#define RX_DONE         3
#define RX_END          4

unsigned int ChkSum(byte*, byte, byte);

byte tx_buf[MAX_TX_PACKT];   //Buffer για δεδομένα εκπομπής LoRa
byte rx_buf[MAX_RX_PACKT];   //Buffer για δεδομένα λήψης LoRa
void sendMessage(String);
boolean reply_en = false;
void onReceive(int);
void onRxTimeOut(void);
void updateLoRa(void);
void ADRReq(byte, byte, byte, byte);
void FREQReq(long f);
void setup_defaults(void);
void eeprom_init(void);

void lora_init()
{
  //=== Αρχικοποίηση πομποδέκτη LORA ===
  LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN0, IRQ_PIN1); // Όρισε τα pins CS, RESET, IRQ0 και IRQ1
  if (!LoRa.begin(frequency))   //Αρχικοποίησε τον πομποδέκτη στη συχνότητα που βρήκες στην EEPROM
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
  LoRa.setSignalBandwidth(bw);            //Το Bandwidth στα 62500 Hz. Ταχύτητα περίπου 2700 bps με ευαισθησία -124dBm
  LoRa.setSpreadingFactor(sf);
  LoRa.setCodingRate4(cr);                //4/5
  LoRa.setTxPower(power);                 //Ισχύς εξόδου +17dBm, Εξ' ορισμού λειτουργεί ο PA με ισχύ από +2 έως +17dBm
  LoRa.setSyncWord(0xc2);                 //το LoRa Wan έχει 0x34 εγώ βάζω C2
  LoRa.enableCrc();
  //Serial.println(LoRa.getSyncWord(), HEX);
  //LoRa.disableCrc();
  LoRa.setRxSymbTimeout(12);              //Αυτό default είναι 100 με 10 * Ts (1.24ms για SF7 - 125Khz ή 4.096 για SF9) ανοίγει παράθυρο ~10msec
                                          //Κανονικά θέλει 12.4 * Ts δηλ. όσο διαρκεί το πρόθεμα, αλλά λειτουργεί και με 6
                                          //Για SF9-125 θα ανοίγει για 50msec και για SF9-62.5 για 100msec
                                          //Με 5 που είναι το ελάχιστο για αναγνώρηση παλαισίου και SF7-250 που ανοίγει παράθυρο για 3,17msec 
                                          //δοκιμάζω την σύμπτωση των παραθύρων
  LoRa.disableInvertIQ();                 //I/Q κανονικά για uplink προς πύλη
  LoRa.idle();                            //Πέσε σε ανενεργή κατάσταση
  Serial.println(F("LoRa init OK!\n"));
}

//========= ISR =======================
//Αν υπάρχουν δεδομένα. Λειτουργία interrupt ISR. Εδώ θα μπει όταν ολοκληρωθεί η λήψη του πακέτου
void onReceive(int packetSize) 
{
  byte i;
  if ((packetSize == 0) || (packetSize > MAX_RX_PACKT)) return; //Αν δεν υπάρχει πακέτο ή είναι πολύ μεγάλο φύγε από εδώ
  //Όταν χρησιμοποιώ callback όπως τώρα, δεν μπορώ να χρησιμοποιήσω καμία συνάρτηση της Stream όπως
  //readString, parseInt() κλπ. Επομένως
  for (i = 0; i < packetSize; ++i)
       rx_buf[i] = LoRa.read();
  rx_state = RX_DONE;
  ii = i;
  //LoRa.idle(); //Δεν είναι απαραίτητη γιατί πάει από μόνο του όταν βρίσκεται στο Rx single
}

//========= ISR =======================
//Αν έγινε timeout λήψης χωρίς να πάρει έγκυρη απάντηση
void onRxTimeOut()
{
 unsigned long d;
 d = micros() - rx_time;
 if (verb > 0)
    {
     Serial.print(F("Timeout after: "));
     Serial.println(d);
    }
 confirmed = false; //Αν δεν έλαβες πλαίσιο από NW τότε μη στείλεις επιβεβαίωση λήψης στο δίκτυο.
 cont++; //Γίνεται 1 μετά το τέλος (timeout) του 1ου παραθύρου και 2 μετά το τέλος του 2ου.
}

//Εκτελείται μέσα από την loop μετά την εκπομπή και αν έχει ληφθεί κάποιο frame σε ένα από τα δύο παράθυρα λήψης
void handleRxFrame()
{
 unsigned int dest_addr, chks, FCntDown;
 byte i;
 i = ii;
 if (rx_buf[0] & 64) //Αν είναι μήνυμα από πύλη δηλ. Downlink
  {
   if (verb > 0)
      {
       Serial.println(F("Rx Frame <"));
       print_table(rx_buf, i, 'h'); //Εμφάνιση κωδικοποιημένων δεδομένων λήψης, το i έχει το μήκος του πακέτου λήψης
      }
   dest_addr =  ((unsigned int) rx_buf[2] * 256) + (unsigned int) rx_buf[1]; //Υπολογισμός διεύθυνσης παραλήπτη
   FCntDown = ((unsigned int) rx_buf[4] * 256) + (unsigned int) rx_buf[3]; //Μετρητής Frame λήψης
   //Το Checksum γίνεται XOR με την διεύθυνση
   chks = ((rx_buf[i-2] ^ rx_buf[1]) + ((rx_buf[i-1] ^ rx_buf[2]) << 8)); //Διάβασε το Check Sum από το τέλος του frame
   //Υπολογισμός CheckSum
   unsigned int s = ChkSum(rx_buf, 0, i - 2);
   if (dest_addr == NODE_ADDRESS) //Αν με αφορά
      {
       rssi = LoRa.packetRssi();
       if (verb > 0)
          {
           Serial.println(F("--- Message for me! ---"));
           Serial.println("  RSSI: " + String(rssi));
           Serial.println("   Snr: " + String(LoRa.packetSnr()));
          }
       //Αν το frame έφτασε σωστά
       if (s == chks)
          {
           if (rx_buf[0] & 128) //Αν από NW ζητήθηκε επιβεβαίωση λήψης τότε ετοίμασε ACK για επόμενο Uplink
               confirmed = true;
           else //Διαφορετικά ήταν σκέτο ACK από το NW και δεν θα θέσει το ACK στο επόμενο Uplink
               confirmed = false;
           //Αν απάντησε ο NW Server σε δικό μου αίτημα επιβεβαίωσης
           if (rx_buf[0] & 16)
              {
               if (verb == 1)
                   Serial.println(F("Ack received from NW Server"));
               Ack_OK = true;
              }
           //Αν υπάρχουν Options μέχρι 8 bytes
           if ((rx_buf[0] & 7) > 0)
              {
               
               byte opt = rx_buf[5]; //Διάβασε το ID
               switch (opt)
                 {
                  case 3: //LinkADRReq
                     ADRReq(rx_buf[6], rx_buf[7], rx_buf[8], rx_buf[9]);
                     break;
                  case 7: //NewFREQReq
                     FREQReq((rx_buf[6] * 65536 + rx_buf[7] * 256 + rx_buf[8]) * 100);
                     break;
                 }
              }
           //Αν υπάρχει μήνυμα δηλ. εντολή από τον Server
           if (i > (8 + (rx_buf[0] & 7))) //Αν το μήκος χωρίς Fopts είναι μεγαλύτερο από 8 τότε υπάρχει μήνυμα διαφορετικά είναι μόνο ACK 
              {
               //Αποκρυπτογράφησε το μήνυμα (είσοδος, έξοδος, αρχή, τέλος) και βάλτο στο rx_message
               decrypt(rx_buf, rx_message, 6 + (rx_buf[0] & 7), i - 2); //το i έχει το μήκος του πακέτου λήψης. Τα 2 τελευταία είναι CheckSum
               if (verb > 0)
                  {
                   Serial.print(F("Message: ")); //Debug
                   Serial.println(FCntDown); //Debug
                  }
               //Εκτέλεση μηνύματος από τις εντολές που υπάρχουν στο commands.h
               //char *instr = (char*) "heater on"; //Για δοκιμή
               strcpy(instr, rx_message); //Αντιγραφή μηνύματος στο instr το οποίο ελέγχεται στο loop ώστε να εκτελεστεί η εντολή
               //cmd_parse(instr); //Ψάξε για εντολή
              }
          }
      else
          if (verb > 0)
              Serial.println(F("*** CheckSum Error ***"));
     }
  }
  cont = 2; //Ενημέρωσε το ClassA_Dev() ότι έγινε λήψη κάποιου frame ώστε να μην προσπαθήσει με επανεκπομπή
  rx_state = RX_END; //Τελείωσε η λήψη. Μη ξαναμπείς εδώ μέχρι τον επόμενο κύκλο
  rx_buf[0] = '\0'; //Η rx_buf είναι κενή
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
 byte i, j, idx;
 for (i = 0; i < MAX_TX_PACKT; ++i)
      tx_buf[i] = 0;
 tx_buf[0] = Type; tx_buf[1] = Address; tx_buf[2] = (Address >> 8); tx_buf[3] = Fcnt; tx_buf[4] = Fcnt >> 8; 
 idx = 5;
 //Αν υπάρχουν options στο header
 if (optnLen > 0)
    {
     for (j = 0; j < optnLen; j++)
          tx_buf[idx++] = options[j]; //Βάλτα στο frame
     optnLen = 0; //Στείλτο μόνο μια φορά
    }
 
 tx_buf[idx++] = Fport; //5 αν δεν υπάρχουν options και μετά να αυξηθεί κατά 1
 for (i=0; i < n; i++)
     {
      tx_buf[idx + i] = Data[i]; //Αντιγραφή δεδομένων στο tx_buf. 6 αν δεν υπάρχουν options
     }
 unsigned int s = ChkSum(tx_buf, 0, i + idx); //6
 tx_buf[i + idx] = s ^ tx_buf[1]; //6
 ++i;
 tx_buf[i + idx] = (s >> 8) ^ tx_buf[2];
 ++i;
 return (i + idx); //Επιστροφή συνολικού μήκους 6
}

//Υπολογισμός CheckSum
unsigned int ChkSum(byte* data, byte start, byte end)
{
 byte i;
 unsigned int s = 0, tmp = 0;
 unsigned long t = 0;
 byte sb;
 for (i = 0; i < end; i += 2) //Για την επικεφαλίδα και όλα τα δεδομένα
     {
      sb = 0;
      if ((i + 1) < end)
          sb = data[i+1];
      tmp = ((unsigned int) (data[i] << 8) + sb);
      s += tmp;
      t += tmp;
      if (t > 0xffff)
         {
          t = s;
          s += 1;  
         }
     }
 return s;
}

//Συνάρτηση αποστολής πακέτου LoRa. Κάνει χρήση των μεθόδων beginPacket, write και endPacket του lora_modem.h
void send_pkt(char* payload, byte n)
{
 byte l, Type;
 //Αν το μήνυμα που έλαβε πριν από τον NW server έχει πληροφορία και ζητάει επιβεβαίωση, τότε να θέσει το ACK
 if (confirmed)
     Type = confRqst | 16 | optnLen; //Βάλε το bit ACK = 1
 else //Διαφορετικά ήταν μόνο ACK και δεν θέλει επιβεβαίωση
     Type = confRqst | optnLen;
 l = prep_pkt(Type, NodeAddress, Fcnt, 0x01, out, n); //Ετοίμασε πακέτο με header 00 ή 80 (χωρίς επιβ. - με επιβ.), διεύθυνση, Α/Α, πόρτα, δεδομένα, μήκος - επιστρέφει μήκος
 if (verb > 0)
     print_table(tx_buf, l, 'h'); //Debug
 LoRa.disableInvertIQ();      //Τα I/Q κανονικά για εκπομπή
 LoRa.beginPacket();          //Ξεκίνα πακέτο
 LoRa.write(tx_buf, l);       //Αντιγραφή δεδομένων μήκους l στο FIFO
 LoRa.endPacket();            //Κλείσε το πακέτο και στείλτο, παράμετρο true αν θέλω ασύγχρονη λειτουργία με interrupt onTxDone
 rx_time = micros();          //Κράτα τον χρόνο ώστε σε λίγο (1sec) να ανοίξει το παράθυρο λήψης
 LoRa.idle();                 //Επιστροφή σε κατάσταση Idle
 rx_state = WAIT_FOR_RX1;
 if (updateLater == 1) updateLater = 2; //Έγινε η εκπομπή επιβεβαίωσης επομένως μπορεί να αλλάξει συχνότητα και DR για την επόμενη φορά
}

//Αλλάζει τις παραμέτρους του πομποδέκτη LoRa
void updateLoRa()
{
 LoRa.setFrequency(frequency);
 LoRa.setSignalBandwidth(bw);            //Το Bandwidth στα 62500 Hz. Ταχύτητα περίπου 2700 bps με ευαισθησία -124dBm
 LoRa.setSpreadingFactor(sf);
 LoRa.setCodingRate4(cr);                //4/5
 LoRa.setTxPower(power);                 //Ισχύς εξόδου +17dBm, Εξ' ορισμού λειτουργεί ο PA με ισχύ από +2 έως +17dBm
}

//--------------------------- Εκτέλεση εντολών MAC που βρίσκονται μέσα στα options του header --------------------------------------------
//Αίτημα για αλλαγή του Data Rate
void ADRReq(byte s_f, byte b_w, byte c_r, byte powr)
{
 sf = (int) s_f;
 switch(b_w) //Το b_w έχει τιμές 0, 1, 2
   {
    case 0:
       bw = 62500; //Hz
       break;
    case 1:
       bw = 125000; //Hz
       break;
    case 2:
       bw = 250000; //Hz
       break;
   }
 cr = (int) c_r;
 power = (int) powr; //ισχύς εκπομπής σε dbm 2-20
 //Ετοίμασε απάντηση
 optnLen = 2; //Απάντηση 2 bytes
 options[0] = 3; //ID: 3 δηλ. ADRAns
 options[1] = 15; //15 - άλλαξαν όλα sf, bw, cr, power δηλ. 00001111
 //updateLoRa(); //Εφάρμοσε αλλαγές στον πομποδέκτη
 updateLater = 1; //Περίμενε να στείλει επιβεβαίωση αλλαγής και άλλαξε για τον επόμενο κύκλο
 Serial.println(F("ADR OK")); //Debug
 optOK = true; //Έγκυρη εντολή. Να γίνει true αν θέλω να στείλω άμεση επιβεβαίωση στον NW server
}

//Αίτημα για αλλαγή συχνότητας λειτουργίας
void FREQReq(long f)
{
 frequency = f;
 //Ετοίμασε απάντηση
 optnLen = 2; //Απάντηση 2 bytes
 options[0] = 7; //ID: 7 δηλ. FREQAns
 options[1] = 1; //1 - άλλαξε η συχνότητα
 //updateLoRa(); //Εφάρμοσε αλλαγές στον πομποδέκτη
 updateLater = 1;
 Serial.print(F("New Freq: ")); //Debug
 Serial.println(f); //Debug
 optOK = true; //Έγκυρη εντολή. Να γίνει true αν θέλω να στείλω άμεση επιβεβαίωση στον NW server
}

//Βάζει προκαθορισμένες τιμές στην EEPROM
void setup_defaults()
{
 EEPROM.put(FREQUENCY_ADDR, FREQUENCY);
 EEPROM.put(SF_ADDR, SF);
 EEPROM.put(BW_ADDR, BW);
 EEPROM.put(CR_ADDR, CR);
 EEPROM.put(POWER_ADDR, POWER);
 EEPROM.put(END1_ADDR, 0xaa);
 EEPROM.put(END2_ADDR, 0x55);
 Serial.println(F("Loading defaults ..."));
}

//Κατά την εκκίνηση διαβάζει ρυθμίσεις από την EEPROM
void eeprom_init()
{
 byte t1, t2, t3;
 EEPROM.get(END1_ADDR, t1);
 EEPROM.get(END2_ADDR, t2);
 if (t1 == 0xaa && t2 == 0x55)
    {
     EEPROM.get(FREQUENCY_ADDR, frequency);
     EEPROM.get(SF_ADDR, t1);
     EEPROM.get(BW_ADDR, bw);
     EEPROM.get(CR_ADDR, t2);
     EEPROM.get(POWER_ADDR, t3);
     sf = (int) t1;
     cr = (int) t2;
     power = (int) t3;
     //Αν η EEPROM είναι κενή δηλαδή έχει 0xFF, βάλε τα defaults. Το ίδιο θα γίνει αν έχουμε φύγει εκτός ορίων
     if (frequency < 410000000 || frequency > 525000000)
         EEPROM.put(FREQUENCY_ADDR, FREQUENCY);
     if (t1 < 7 || t1 > 12)
         EEPROM.put(SF_ADDR, SF);
     if (bw < 62500 || bw > 250000)
         EEPROM.put(BW_ADDR, BW);
     if (t2 < 5 || t2 > 8)
         EEPROM.put(CR_ADDR, CR);
     if (t3 > 20 || t3 < 2)
         EEPROM.put(POWER_ADDR, POWER);
    }
 else
     setup_defaults();
}