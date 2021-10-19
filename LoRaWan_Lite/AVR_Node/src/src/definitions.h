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

#define DIN_W1T1               0  //Τερματικός T1 για παράθυρο 1 - Ανοιχτό
#define DIN_W1T2               1  //Τερματικός T2 για παράθυρο 1 - Κλειστό
#define DIN_W2T1               2  //Τερματικός T1 για παράθυρο 2 - Ανοιχτό
#define DIN_W2T2               3  //Τερματικός T2 για παράθυρο 2 - Κλειστό
#define DIN_SKT1               4  //Τερματικός T1 για κουρτίνα σκίασης - Μαζεμένη
#define DIN_SKT2               5  //Τερματικός T2 για κουρτίνα σκίασης - Απλωμένη
#define DIN_MOIST              6  //Στάθμη νερού πλήρης
//---------------------------- 7  //To 7 είναι διαθέσιμο για ψηφιακή είσοδο

#define DOUT_LAMP              0  //Θερμαντικό σώμα
#define DOUT_PUMP              1  //Αντλία ποτίσματος
#define DOUT_W1_C1             2  //Παράθυρο 1 επαφή 1
#define DOUT_W1_C2             3  //Παράθυρο 1 επαφή 2
#define DOUT_W2_C1             4  //Παράθυρο 2 επαφή 1
#define DOUT_W2_C2             5  //Παράθυρο 2 επαφή 2
#define DOUT_SK_C1             6  //Κουρτίνα σκίασης επαφή 1
#define DOUT_SK_C2             7  //Κουρτίνα σκίασης επαφή 2

#define clockPulse()  {digitalWrite(SCLK_PIN, HIGH);digitalWrite(SCLK_PIN, LOW);}


#define MSG_LEN               128

#define NODE_ADDRESS          0xAB31
#define NO_ACK                0x00
//#define CONFIRM_REQ           0x00  //Unconfirmed
#define CONFIRM_REQ           0x80  //Confirmed

//Για λειτουργία Watch Dog
#define INTERVAL              8   //Κάθε 60 sec / 8 = 7.5 = 8   ---- 8 * 8 = 64 sec

#define INTERVAL_MS           60000
#define RX1_TIME              1000000 //1sec
#define RX2_TIME              2000000 //2sec
#define RX1_OPEN_TOLLER       100 //μsec ο χρόνος που κάνει να εκτελεστεί η receive_single()
#define TX_RETRIES            3   //Είναι το NbTrans

#define FREQ_START            433050000
#define FREQ_END              434790000   //Εύρος 1.74MHz δηλαδή 28 κανάλια 62.5KHz
#define FREQUENCY             433290000   //Προκαθορισμένη συχνότητα

//------- Blink Codes ----------------
#define COM_STATE_OK          0
#define SETUP_ERROR           1
#define NO_REPLY_ERROR        2
#define TX_ENABLE             3

const byte enc_key[] = {0x21, 0x02, 0xf8, 0xaf, 0x88, 0x00, 0x09, 0x6e}; //Κλειδί 64bit

long frequency;
byte error_code = 0, error_code_old = 0;
byte dig[] = {0, 0, 0, 0};

int rssi = 0; //Ένταση σήματος
unsigned long rx_time; //Ο μετρητής για το άνοιγμα του παραθύρου λήψης
unsigned long interval;
byte rx_state;
unsigned int Fcnt; //Α/Α πλαισίου
const unsigned int NodeAddress = NODE_ADDRESS; //Η φυσική διεύθυνση αυτού του κόμβου
unsigned long d;
byte payload_len;

//Αυτές είναι volatile γιατί αλλάζουν μέσα σε Interrupts
volatile bool f_wdt = true; //Αν είναι False δεν μπαίνει στη loop και παραμένει σε κατάσταση PWR Down
volatile byte cont = 0; //Όταν είναι 0 περιμένει για λήψη, 1 τελείωσε το RX1 με timeout, 2 τελείωσε το RX2 ή έλαβε πλαίσιο Rx
//------------------------------------------------------

bool send_flag = false; //Όταν είναι True έκανε εκπομπή και περιμένει για τα παράθυρα λήψης
bool confirmed = false; //Αν ζητήθηκε επιβεβαίωση λήψης από το NW Server τότε είναι True
bool sample_flag;       //Όταν είναι True ετοιμάζεται το επόμενο δείγμα - πακέτο για εκπομπή
bool Ack_OK = false;    //Αν ο NW Server απάντησε με Ack για το προηγούμενο πλαίσιο
byte Retr_count = 0;    //Μετρητής επανεκπομπής
byte randDelay;         //Τυχαία καθυστέρηση σε ms για επανεκπομπή πλαισίου

float t_int, max_t_int = 0.0;
float dht_temp = 0.0, max_dht_temp = -40.0, min_dht_temp = 100.0;
float T_int_S = 0.0, dht_temp_S = 0, humidity_S = 0; //Αθροίσματα τιμών κάθε 1sec
float humidity = 0.0, max_humidity = -50.0, min_humidity = 200.0; 

byte dout_status = 0;
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