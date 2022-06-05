#define VERSION                "1.0 beta"

//======================= Ορισμοί θυρών του Arduino =========================================
#define STATUS_LED             9
#define LED1                  13
#define SDATA_IN_PIN          19  //A5-Διαβάζει σειριακά δεδομένα από το HC597
#define SCLK_PIN               7  //Το pin SHCP του HC597 και για το HC595
#define STCP_PIN               4  //Το pin STCP ή Parallel Load του HC597
#define SDATA_OUT_PIN          4  //Το pin Data Serial για το HC595 είναι ίδιο με πάνω
#define P_LOAD                 8  //Το pin STCP ή Parallel Load του HC595
#define FAN_PIN                5  //Το PWM pin για ανεμιστήρες
#define PH_PIN                A0  //Αναλογική είσοδος για μέτρηση Ph
#define IRRAD_PIN             A1  //Αναλογική είσοδος για ένταση ηλιακής ακτινοβολίας
#define WINDOW_SPEED_PIN       6  //Το PWM pin για ταχύτητα παραθύρων

//====================== Ορισμοί εισόδων του Port Expander ===================================
#define DIN_W1T1               0  //Τερματικός T1 για παράθυρο 1 - Ανοιχτό ή 'A' στην κλέμα
#define DIN_W1T2               1  //Τερματικός T2 για παράθυρο 1 - Κλειστό ή 'B' στην κλέμα
#define DIN_W2T1               2  //Τερματικός T1 για παράθυρο 2 - Ανοιχτό ή 'A' στην κλέμα
#define DIN_W2T2               3  //Τερματικός T2 για παράθυρο 2 - Κλειστό ή 'B' στην κλέμα
#define DIN_SKT1               4  //Τερματικός T1 για κουρτίνα σκίασης - Μαζεμένη
#define DIN_SKT2               5  //Τερματικός T2 για κουρτίνα σκίασης - Απλωμένη
#define DIN_MOIST              6  //Στάθμη νερού πλήρης
//---------------------------- 7  //To 7 είναι διαθέσιμο για ψηφιακή είσοδο

//====================== Ορισμοί εξόδων του Port Expander ====================================
#define DOUT_LAMP              0  //Θερμαντικό σώμα
#define DOUT_PUMP              1  //Αντλία ποτίσματος
#define DOUT_W1_C1             2  //Παράθυρο 1 επαφή 1
#define DOUT_W1_C2             3  //Παράθυρο 1 επαφή 2
#define DOUT_W2_C1             4  //Παράθυρο 2 επαφή 1
#define DOUT_W2_C2             5  //Παράθυρο 2 επαφή 2
#define DOUT_SK_C1             6  //Κουρτίνα σκίασης επαφή 1
#define DOUT_SK_C2             7  //Κουρτίνα σκίασης επαφή 2

//Ορισμός του Clk Pulse για shift registers
#define clockPulse()  {digitalWrite(SCLK_PIN, HIGH);digitalWrite(SCLK_PIN, LOW);}


#define MSG_LEN               80 //Μέγιστο μήκος μηνύματος
#define NODE_ADDRESS          0xAB31 //Διεύθυνση του κόμβου
#define NO_ACK                0x00
#define UNCONFIRM             0x00  //Unconfirmed
#define CONFIRM_REQ           0x80  //Confirmed

//Ορισμοί χρονιστών λειτουργίας του συστήματος
//#define AQUIRE_MS             25000   //Κάθε 25 δευτερόλεπτα (σε msec) διαβάζει αισθητήρες
//Κύκλος εκπομπής μηνυμάτων από 10sec έως 1hour - καλό είναι κάθε 1 λεπτό
#define TX_CYCLE              ((1 * 60000) - 2000) //Κύκλος χρήσης LORA κάθε 1 λεπτό (εκπομπή 0,1% ή 0,6 για SF9)
#define AQUIRE_MS             TX_CYCLE //π.χ. στα 58 sec διάβασε κατάσταση αισθητήρων
#define LORA_PREP_MS          (TX_CYCLE + 1000)  //Κάθε n λεπτά και 1 sec (σε msec) προετοίμασε πακέτο για εκπομπή
#define LORA_TX_MS            (TX_CYCLE + 2000)  //Κάθε n λεπτά και 2 sec (σε msec) Στείλε το πακέτο
#define UPTIME_SEC            (LORA_TX_MS / 1000)

//Χρόνοι LoRaWan Lite 
#define RX1_TIME              1000000 //1sec (σε μsec) το 1ο παράθυρο λήψης
#define RX2_TIME              2000000 //2sec (σε μsec) το 2ο παράθυρο λήψης
#define RX1_OPEN_TOLLER       50 //μsec ο χρόνος που κάνει να εκτελεστεί η receive_single()
#define TX_RETRIES            3   //Είναι το NbTrans δηλ. επανάληψη πακέτου αν δεν το λάβει ο server

//Συχνότητες λειτουργίας
#define FREQ_START            433050000
#define FREQ_END              434790000   //Εύρος 1.74MHz δηλαδή 28 κανάλια 62.5KHz
#define FREQUENCY             433150000   //Προκαθορισμένη συχνότητα λειτουργίας
/* Αρχικό κανάλι βάζω το 433.150 και μετά αυξάνω ανά 200Khz δηλ. 433.350, 433.550, 433.750, 433.950, 434.150, 434.350, 434.550, 434.750
δηλαδή έχω 9 κανάλια
*/

#define EEPROM_START          0  //Αρχική διεύθυνση της EEPROM
#define FREQUENCY_ADDR        EEPROM_START                  //Αποθηκεύεται η συχνότητα
#define SF_ADDR               FREQUENCY_ADDR + sizeof(long) //Αποθηκεύεται το sf
#define BW_ADDR               SF_ADDR + sizeof(byte)        //Αποθηκεύεται το bw
#define CR_ADDR               BW_ADDR + sizeof(long)        //Αποθηκεύεται το cr 4/5, 4/6, 4/7, 4/8
#define POWER_ADDR            CR_ADDR + sizeof(byte)        //Αποθηκεύεται η ισχύς 2-20 dBm
#define END1_ADDR             POWER_ADDR + sizeof(byte)     //Τέλος
#define END2_ADDR             END1_ADDR + sizeof(byte)

//------- Blink Codes ----------------
#define COM_STATE_OK          0
#define SETUP_ERROR           1
#define NO_REPLY_ERROR        2
#define TX_ENABLE             3

//Ορισμός κλειδιού κωδικοποίησης του μηνύματος
const byte enc_key[] = {0x21, 0x02, 0xf8, 0xaf, 0x88, 0x00, 0x09, 0x6e}; //Κλειδί 64bit

//Μεταβλητές LoRa
long frequency, bw;
int sf, cr, power;

byte error_code = 0, error_code_old = 0;
byte dig[] = {0, 0, 0, 0};
int rssi = 0; //Ένταση σήματος
unsigned long rx_time; //Ο μετρητής για το άνοιγμα του παραθύρου λήψης
volatile byte rx_state;
unsigned int Fcnt; //Α/Α πλαισίου
const unsigned int NodeAddress = NODE_ADDRESS; //Η φυσική διεύθυνση αυτού του κόμβου
unsigned long d;
byte payload_len;
volatile byte ii, updateLater = 0;

//Μεταβλητές λειτουργίας LoRaWan Lite
//Αυτές είναι volatile γιατί αλλάζουν μέσα σε Interrupts
//volatile bool f_wdt = false; //Αν είναι False δεν μπαίνει στη loop και παραμένει σε κατάσταση PWR Down
char instr[40];
volatile byte cont = 0; //Όταν είναι 0 περιμένει για λήψη, 1 τελείωσε το RX1 με timeout, 2 τελείωσε το RX2 ή έλαβε πλαίσιο Rx
bool time_4_tx = false;
bool send_flag = false; //Όταν είναι True έκανε εκπομπή και περιμένει για τα παράθυρα λήψης
bool confirmed = false; //Αν ζητήθηκε επιβεβαίωση λήψης από το NW Server τότε είναι True
byte confRqst;          //Αν ζητάει ο κόμβος επιβεβαίωση από το δίκτυο
byte optnLen;           //Μήκος δεδομένων option στην επικεφαλίδα μέχρι 8
byte options[8];        //Το περιεχόμενο των options    
bool optOK;     
volatile bool Ack_OK = false;    //Αν ο NW Server απάντησε με Ack για το προηγούμενο πλαίσιο
byte Retr_count = 0;    //Μετρητής επανεκπομπής
byte randDelay;         //Τυχαία καθυστέρηση σε ms για επανεκπομπή πλαισίου

//Μεταβλητές χρονιστών λειτουργίας
unsigned long interval1; //Κάθε πότε κάνει μέτρηση από αισθητήρες
unsigned long interval2; //Κάθε πότε προετοιμάζει το μήνυμα LORA για εκπομπή
unsigned long interval3; //Κάθε πότε εκπέμπει μήνυμα LORA
unsigned long uptime;
bool imed_flag = false;

//Μεταβλητές αισθητήρων θερμοκρασίας - υγρασίας - Ph - Irradiation
float t_ext, max_t_ext = 0.0;
float dht_temp = 0.0, max_dht_temp = -40.0, min_dht_temp = 100.0;
float T_ext_S = 0.0, dht_temp_S = 0, humidity_S = 0; //Αθροίσματα τιμών κάθε 1sec
float humidity = 0.0, max_humidity = -50.0, min_humidity = 200.0; 
float ph;
unsigned int irrad;
String t_ext_str, t_int_str, hum_str, ph_str, irrad_str, status_str;

//Μεταβλητές και εντολές εξόδου του port expander
byte dout_status, old_dout_status;
byte din_status, old_din_status;
#define LAMP_ON()     { dout_status |= (1 << DOUT_LAMP); }
#define LAMP_OFF()    { dout_status &= ~(1 << DOUT_LAMP); }

#define PUMP_ON()     { dout_status |= (1 << DOUT_PUMP); }
#define PUMP_OFF()    { dout_status &= ~(1 << DOUT_PUMP); }

#define W1_OPEN()     { dout_status |= (1 << DOUT_W1_C2); dout_status &= ~(1 << DOUT_W1_C1); }
#define W1_CLOSE()    { dout_status |= (1 << DOUT_W1_C1); dout_status &= ~(1 << DOUT_W1_C2); }
#define W1_STOP()     { dout_status &= ~(1 << DOUT_W1_C1); dout_status &= ~(1 << DOUT_W1_C2); }

#define W2_OPEN()     { dout_status |= (1 << DOUT_W2_C2); dout_status &= ~(1 << DOUT_W2_C1); }
#define W2_CLOSE()    { dout_status |= (1 << DOUT_W2_C1); dout_status &= ~(1 << DOUT_W2_C2); }
#define W2_STOP()     { dout_status &= ~(1 << DOUT_W2_C1); dout_status &= ~(1 << DOUT_W2_C2); }

#define SKIA_OPEN()   { dout_status |= (1 << DOUT_SK_C2); dout_status &= ~(1 << DOUT_SK_C1); }  //Μαζεύει
#define SKIA_CLOSE()  { dout_status |= (1 << DOUT_SK_C1); dout_status &= ~(1 << DOUT_SK_C2); }  //Απλώνει
#define SKIA_STOP()   { dout_status &= ~(1 << DOUT_SK_C1); dout_status &= ~(1 << DOUT_SK_C2); }

#define MAX_WIN_TIME          3000 // 3 sec
#define MAX_SKIA_TIME        12000 //12 sec
#define MAX_POTISMA_TIME    300000 //5 λεπτά
#define MAX_INT_TEMP          25.0

byte win_speed; //Ταχύτητα κίνησης παραθύρων
byte air_idx = 0; //Κλίμακες έντασης από 0 - 5
byte air_stength; //Ένταση περιστροφής εξαερισμού

byte verb = 0;