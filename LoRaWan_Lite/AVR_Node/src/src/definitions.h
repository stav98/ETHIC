#define STATUS_LED            4
#define LED1                  7
#define MSG_LEN               128

#define NODE_ADDRESS          0xAB31
#define NO_ACK                0x00
//#define CONFIRM_REQ           0x00  //Unconfirmed
#define CONFIRM_REQ           0x80  //Confirmed

#define INTERVAL              8       //Κάθε 60 sec / 8 = 7.5 = 8   ---- 8 * 8 = 64 sec
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
