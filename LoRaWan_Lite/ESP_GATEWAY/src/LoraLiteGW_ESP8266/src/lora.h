#define CS_PIN                        0     //16
#define RESET_PIN                    -1    //Δεν συνδέεται
#define IRQ_PIN0                      5

#define SF                            7
#define BW                       125000     //Hz
//#define SF                           12
//#define BW                        62500     //Hz
#define FREQUENCY             433290000
#define FREQ_START            433050000
#define FREQ_END              434790000   //Εύρος 1.74MHz δηλαδή 28 κανάλια 62.5KHz

#define RX                    0
#define TX                    1

void onReceive(int);
void push_data(void);  //Ορίζεται στο network.h

byte incoming[240];
unsigned long tx_time;
//unsigned int frequency = F1;
unsigned int frequency = FREQUENCY;
byte tx_state = 0;
int rssi = 0; //Ένταση σήματος
int snr = 0;
int msgLen = 0;
int Tx_Bw;


void initLoRa()
{
  LoRa.setPins(CS_PIN, RESET_PIN, IRQ_PIN0);
  while (!LoRa.begin(frequency)) 
        {
         Serial.println(".");
         delay(500);
        }
  LoRa.onReceive(onReceive);              //Βάλε ISR για λήψη
  LoRa.setSyncWord(0xc2);                 //0x34 για LORAWAN
  //Serial.println("LoRa Initializing OK!");
  LoRa.setSignalBandwidth(BW);            //Το Bandwidth στα 62500 Hz. Ταχύτητα περίπου 2700 bps με ευαισθησία -124dBm
  LoRa.setSpreadingFactor(SF);
  LoRa.setTxPower(17);                    //Ισχύς εξόδου +17dBm, Εξ' ορισμού λειτουργεί ο PA με ισχύ από +2 έως +17dBm
  //LoRa.writeRegister(0x33, 0x27);
  LoRa.disableInvertIQ();                 //Αρχικά για λήψη τα I/Q είναι κανονικά
  LoRa.receive(); 
}

//*** Λήψη πακέτου LoRa από τα Nodes ***
void onReceive(int packetSize) 
{
  //boolean valid_message = false;
  if (packetSize == 0) return;          //Αν δεν υπάρχει πακέτο φύγε από εδώ
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
  if (!(incoming[0] & 64)) //Αν το μήνυμα προέρχεται από κόμβο προς εδώ Uplink
     {
      msgLen = packetSize;
      Serial.print(F("* < Message from node: "));
      Serial.print(incoming[2], HEX);
      Serial.println(incoming[1], HEX);
      print_table(incoming, packetSize, 'h'); //Debug
      rssi = LoRa.packetRssi();
      snr = LoRa.packetSnr();
      Serial.println("RSSI: " + String(rssi));
      Serial.println("Snr: " + String(LoRa.packetSnr()));
      //int c = encode_base64(incoming, packetSize, (byte*)ascii_data); //Σε περίπτωση που θέλουμε κωδικοποίηση Base64
      //Serial.println(ascii_data);
      push_data(); //Στείλε τα δεδομένα στον NW Server
     }
}
