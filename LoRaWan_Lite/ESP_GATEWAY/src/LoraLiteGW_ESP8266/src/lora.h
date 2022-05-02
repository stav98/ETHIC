#define CS_PIN                        0     //16
#define RESET_PIN                    -1    //Δεν συνδέεται
#define IRQ_PIN0                      5

#define RX                    0
#define TX                    1
#define MAX_RX_PACKT          240
void onReceive(int);
unsigned short ChkSum(byte*, byte, byte);
void push_data(void);  //Ορίζεται στο network.h

byte incoming[MAX_RX_PACKT]; //Buffer για LoRa Rx
unsigned long tx_time;
unsigned int frequency;
byte tx_state = 0;
int rssi = 0; //Ένταση σήματος
int snr = 0;
int msgLen = 0;
int Tx_Bw;

void initLoRa()
{
  LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN0);
  frequency = lora_freq.toInt();
  while (!LoRa.begin(frequency)) 
        {
         Serial.println(".");
         delay(500);
        }
  LoRa.onReceive(onReceive);              //Βάλε ISR για λήψη
  LoRa.setSyncWord(lora_sync.toInt());                 
  LoRa.setSignalBandwidth(lora_bw.toInt());            //Το Bandwidth στα 62500 Hz. Ταχύτητα περίπου 2700 bps με ευαισθησία -124dBm
  LoRa.setSpreadingFactor(lora_sf.toInt());
  LoRa.setCodingRate4(lora_cr.toInt());
  LoRa.setTxPower(lora_power.toInt());                    //Ισχύς εξόδου +17dBm, Εξ' ορισμού λειτουργεί ο PA με ισχύ από +2 έως +17dBm
  //LoRa.writeRegister(0x33, 0x27);
  LoRa.disableInvertIQ();                 //Αρχικά για λήψη τα I/Q είναι κανονικά
  LoRa.receive(); 
}

//*** Λήψη πακέτου LoRa από τα Nodes ***
void onReceive(int packetSize) 
{
  //boolean valid_message = false;
  if ((packetSize == 0) || (packetSize > MAX_RX_PACKT)) return;          //Αν δεν υπάρχει πακέτο φύγε από εδώ
  //Τα δεδομένα του μηνύματος
  //String incoming = "";                 //Το καθαρό φορτίο του μηνύματος
  
  //Όταν χρησιμοποιώ callback όπως τώρα, δεν μπορώ να χρησιμοποιήσω καμία συνάρτηση της Stream όπως
  //readString, parseInt() κλπ. Επομένως
  //while (LoRa.available())   //Όσο υπάρχουν δεδομένα διαθέσιμα στο buffer
         //incoming += (char)LoRa.read();      //Πρόσθεσε τα bytes ένα - ένα στο string  
  //       incoming[cnt++] = LoRa.read();
  tx_time = micros(); //Πάρε τον χρόνο για το παράθυρο rx1
  for (byte i = 0; i < packetSize; i++) 
       incoming[i] = LoRa.read();
  tx_state = TX; //Ενεργοποίησε κατάσταση εκπομπής από GW προς Node
  if (!(incoming[0] & 64)) //Αν το μήνυμα προέρχεται από κόμβο προς εδώ Uplink flag uplink
     {
      msgLen = packetSize;
      Serial.print(F("* < Message from node: "));
      Serial.print(incoming[2], HEX);
      Serial.println(incoming[1], HEX);
      //print_table(incoming, packetSize, 'h'); //Debug
      rssi = LoRa.packetRssi();
      snr = LoRa.packetSnr();
      unsigned short frameId = incoming[4] * 256 + incoming[3];
      //Serial.println("RSSI: " + String(rssi)); //Debug
      //Serial.println("Snr: " + String(LoRa.packetSnr())); //Debug
      //Επαλήθευση σωστού CheckSum
      unsigned short s = ChkSum(incoming, 0, msgLen-2);
      byte a = s & 0xff;
      byte b = s >> 8;
      a ^= incoming[1]; 
      b ^= incoming[2];
      //Serial.println(rx_pkts_str); //Debug
      //int c = encode_base64(incoming, packetSize, (byte*)ascii_data); //Σε περίπτωση που θέλουμε κωδικοποίηση Base64
      //Serial.println(ascii_data);
      char chksumSt = 'e'; //Κατάσταση ορθότητας πλαισίου στο web interface
      //Αν είναι σωστό το checksum τότε κάνε Push Data
      if (a == incoming[msgLen-2] && b == incoming[msgLen-1])
         {
          push_data(); //Στείλε τα δεδομένα στον NW Server
          chksumSt = 'C'; //Βάλε σήμανση σωστού Correct checksum
          rxpkt_cnt++;
         }
      //Φτιάχνει ένα string με πληροφορίες για το πακέτο που έλαβε ώστε να εμφανιστεί στο web dashboard
      sprintf(rx_pkt_log, "%s | %02X%02X | %04X | %03d | %.3d | %c <br>\n",curTime().c_str(), incoming[2], incoming[1], frameId, msgLen, rssi, chksumSt);
      //Μετακίνησε τις γραμμές κατά μια θέση σβήνοντας την παλαιότερη γραμμή
      for (byte i = 0; i < LOG_LINES-1; i++)
           rx_pkts_log[i] = rx_pkts_log[i+1];
      //Βάλε στην τελευταία γραμμή τα νεότερα δεδομένα
      rx_pkts_log[LOG_LINES-1] = String(rx_pkt_log);
      //Φτιάξε το τελικό string το οποίο θα στέλνει στο web interface
      rx_pkts_str = "";
      for (byte i = 0; i < LOG_LINES; i++)
           rx_pkts_str += rx_pkts_log[i];
     }
}

//Υπολογισμός CheckSum 16 bit
unsigned short ChkSum(byte* data, byte start, byte end)
{
 byte i;
 unsigned short s = 0;
 unsigned int t = 0;
 for (i = 0; i < end; i += 2) //Για την επικεφαλίδα και όλα τα δεδομένα
     {
      s += ((unsigned short) (data[i] << 8) + data[i+1]); 
      t += ((unsigned short) (data[i] << 8) + data[i+1]); 
      if (t > 0xffff)
         {
          t = s;
          s += 1;  
         }
     }
 return s;
}