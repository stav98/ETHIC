byte keys[16][8]; 

char out[MSG_LEN + 1]; //Εδώ μπαίνει το κωδικοποιημένο μήνυμα προς εκπομπή
char rx_message[MSG_LEN + 1]; //Buffer με το αποκωδικοποιημένο μήνυμα που έλαβε από LoRa

//Αρχικός πίνακας μετάθεσης για το κλειδί
const byte CP_1[] PROGMEM = {
         8,  8,                                   
        57, 49, 41, 33, 25, 17,  9,  8,
         1, 58, 50, 42, 34, 26, 18, 40,
        10,  2, 59, 51, 43, 35, 27, 24,
        19, 11,  3, 60, 52, 44, 36, 56,
        63, 55, 47, 39, 31, 23, 15, 16,
         7, 62, 54, 46, 38, 30, 22, 32, 
        14,  6, 61, 53, 45, 37, 29, 48,
        21, 13,  5, 28, 20, 12,  4, 64
};

//Αρχικός πίνακας μετάθεσης για τα δεδομένα
const byte PI1[] PROGMEM = {
         8,  8,
      //b0  b1  b2  b3  b4  b5  b6  b7                                      
        58, 50, 42, 34, 26, 18, 10, 2,
      //b8  b9  b10
        60, 52, 44, 36, 28, 20, 12, 4,
        62, 54, 46, 38, 30, 22, 14, 6,
        64, 56, 48, 40, 32, 24, 16, 8,
        57, 49, 41, 33, 25, 17,  9, 1,
        59, 51, 43, 35, 27, 19, 11, 3,
        61, 53, 45, 37, 29, 21, 13, 5,
        63, 55, 47, 39, 31, 23, 15, 7
};

//Αντίστροφος του αρχικού πίνακα μετάθεσης για τα δεδομένα
const byte PI2[] PROGMEM = {
         8, 8,
        40, 8, 48, 16, 56, 24, 64, 32,
        39, 7, 47, 15, 55, 23, 63, 31,
        38, 6, 46, 14, 54, 22, 62, 30,
        37, 5, 45, 13, 53, 21, 61, 29,
        36, 4, 44, 12, 52, 20, 60, 28,
        35, 3, 43, 11, 51, 19, 59, 27,
        34, 2, 42, 10, 50, 18, 58, 26,
        33, 1, 41,  9, 49, 17, 57, 25
};

//Πίνακας μετάθεσης για παραγωγή κλειδιών
const byte CP_2[] PROGMEM = {
         8,  8,
        14, 17, 11, 24,  1,  5,  3, 28,
        15,  6, 21, 10, 23, 19, 12,  4,
        26,  8, 16,  7, 27, 20, 13,  2,
        41, 52, 31, 37, 47, 55, 30, 40,
        51, 45, 33, 48, 44, 49, 39, 56,
        34, 53, 46, 42, 50, 36, 29, 32,
        64, 61, 63, 62, 57, 60, 58, 59,
         9, 18, 22, 25, 35, 38, 43, 54 };

//Οδηγίες ολίσθησης για παραγωγή κλειδιών
const byte SHIFT[] PROGMEM = { 1, 1, 2, 2, 3, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

//Οδηγίες ολίσθησης για κωδικοποίηση δεδομένων σε 16 γύρους, το αρνητικό είναι Shift Right και το θετικό Left.
const int SHIFT1[] PROGMEM = { 0, 1, 2, 2, 3, 4, 5, 6, 0, -1, -2, -3, -4, -6, -5, 7 };

//Αντιγράφει το αρχικό κείμενο και προσθέτει padding στο τέλος βάσει του PKCS5
void addPadding(char* data, char* res)
{
  byte pad_len = 8 - (strlen(data) % 8);
  byte i = 0, j = 0;
  res[0]= '\0';
  while (data[i] != '\0')
        {
         res[i] = data[i];
         i++;
        }
       //res[i++] = data[i];
  if (pad_len < 8)
      for (j = 0; j < pad_len; j++)
           res[i++] = pad_len;
  res[i] = '\0';
  //print_table(res, 56, 'h'); //Debug
}

//Αφαιρεί padding αν υπάρχει
void removePadding(char* data)
{
 byte i = 0;
 byte len = strlen(data);
 byte pad_len = data[len-1];
 if (pad_len < 8)
     for (i = len; i >= (len - pad_len); --i)
          data[i] = '\0';
}

//Ανακατεύει τα bits του block in (8 bytes), σύμφωνα με τις θέσεις του πίνακα ptable και το αποτέλεσμα πάει στο out
void permute(const byte *ptable, const byte *in, byte *out)
{
 byte ob; //πλήθος των bytes εξόδου
 byte bytes, bits; //μετρητές για bits και bytes
 ob = pgm_read_byte(&ptable[1]); //Διάβασε από πίνακα το πλήθος των bytes
 ptable = &(ptable[2]); //Δείξε στο πρώτο στοιχείο του πίνακα 8x8
 for (bytes = 0; bytes < ob; ++bytes)
     {
      byte x, t = 0;
      for (bits = 0; bits < 8; ++bits)
          {
           x = pgm_read_byte(ptable++) - 1;
           t <<= 1; //ολίσθηση αριστερά
           //Αν το συγκεκριμένο bit από το byte του in είναι '1'
           if ((in[x / 8]) & (0x80 >> (x % 8)))
               t |= 0x01; //τότε να γίνει το αποτέλεσμα 1
          }
      out[bytes]=t;
     }
}

//Ολισθαίνει αριστερά τα 64 bits κατά μία θέση 
void sl(byte *out)
{
 byte carry = 0; byte carry_pre = 0;
 int i;
 for(i = 7; i >= 0; --i)
    {
      if (out[i] & 0b10000000)
         carry = 1;
      else
         carry = 0;
      out[i] <<= 1;
      out[i] |= carry_pre;
      carry_pre = carry;
    }
 out[7] |= carry_pre;
}

//Καλεί την sl τόσες φορές ώστε να γίνει η επιθυμητή ολίσθηση. Για τιμές 63, 62, 61 κλπ. κάνει ολίσθηση προς τα δεξιά
void shift(byte *out, byte n) 
{
  byte i;
  if(n > 248) //Είναι αρνητικός επομένως θα κάνει Shift Right
     n = n - 192; //63 για -1, 62 για -2 κ.ο.κ.
  for(i = 0; i < n; ++i)
      sl(out);
}

//Κάνει XOR σε 8byte με κάποιο κλειδί 8byte.
//Το αποτέλεσμα επιστρέφει στο data
void xor_tbl(byte *data, byte *pat)
{
  byte i;
  for(i = 0; i < 8; ++i)
      data[i] ^= pat[i];
}

//Δημιουργεί 16 κλειδιά 64bit από το αρχικό enc_key
void keys_generator()
{
 byte i, k[8], l[8];
 permute((byte*)CP_1, (const byte*)enc_key, k); //Μετακίνηση των bit βάσει του πίνακα CP_1 και αντιγραφή στον πίνακα k
 //Παραγωγή 16 κλειδιών
 for(i = 0; i < 16; ++i)
    {
     shift(k, pgm_read_byte(&SHIFT[i])); //Ολίσθηση του κλειδιού k βάσει του πίνακα SHIFT
     permute((byte*)CP_2, k, l); //Μετάθεση των bits κατά τον πίνακα CP_2
     //print_table(l, 8); //Debug
     memcpy(keys[i], l, 8); //Αποθήκευση κλειδιών στον πίνακα keys[16]
    }
}

//========== Κωδικοποίηση μηνύματος ========== //και επιστροφή character array σε BASE64
//void encrypt(byte *msg, byte *out)
byte encrypt(byte *msg, byte *out)
{
 byte idx = 0, i, j, l[8];
 char s[MSG_LEN + 1], b[9];
 //Κρυπτογράφηση δεδομένων
 s[0]='\0'; //Αρχικά το byte array να είναι κενό
 //Serial.println(msg); //Debug
 addPadding((char*) msg, s); //Αντέγραψε το μήνυμα και βάλε padding σε περίπτωση που δεν είναι ακέραιο πολ/σιο του 8
 //Serial.println(s); //Debug
 byte blocks = (byte) strlen(s) / 8;  //Υπολόγισε τα blocks μαζί με το padding 
 for(i = 0; i < blocks; ++i) //Για κάθε block
    {
     for(j = 0; j < 8; ++j) //Για κάθε χαρακτήρα του block
         b[j] = s[i * 8 + j]; //Φτιάξε οκτάδες
     b[8] = '\0'; //Σε περίπτωση που θα χρειαστεί εκτύπωση βάλε στον 9ο χαρακτήρα \0
     //print_table(b, 8, 'h'); 
     permute((byte*)PI1, (byte*)b, l); //Μετάθεση των bit του block βάσει του πίνακα PI1 
     b[0] = '\0'; //Κενό για την επόμενη φορά
     for(j = 0; j < 16; ++j)
         {
          xor_tbl(l, keys[j]); //Κάνε XOR τα 8 bytes που έφτιαξες πριν με την μετάθεση με τα κλειδιά κωδικοποίησης που έχεις δημιουργήσει πριν
          shift(l, pgm_read_byte(&SHIFT1[j])); //Ολίσθησε το προηγούμενο σύμφωνα με το στοιχείο του πίνακα SHIFT1
          //print_table(l, 8, 'h'); //Debug
         }
      //Εδώ πρέπει οι 8άδες να γίνουν ένα byte array προς μετάδοση
      //print_table(l, 8, 'h'); //Debug
      for(j = 0; j < 8; ++j)
          out[idx++] = l[j];      
     }
  out[idx] = '\0';
  //print_table(out, 56, 'h'); //Debug
  //int c = encode_base64(out, strlen(out), s); //Σε περίπτωση που θέλουμε κωδικοποίηση Base64
  //memcpy(out, s, strlen(s));  //Αυτή η γραμμή ενεργοποιείται αν θέλω Base64
  return idx;
}

//Αποκρυπτογράφιση απλοποιημένου DES 
//inp: δεδομένα εισόδου, out: δεδομένα εξόδου, mlen: μήκος
void decrypt(byte *inp, char *out, byte start, byte mlen)
{
 byte i, j, k, idx = 0, l[8]; 
 char o[9];
 k = 0;
 for (i = start; i < mlen; i++) //Από το 6 ξεκινάει η κωδικοποιημένη πληροφορία όταν δεν έχει options
     {   
      l[k++] = inp[i]; //Χώρισε σε οκτάδες   
      if (k > 7) //Αν συμπληρώθηκε οκτάδα
         {
          //print_table(l, 8, 'h'); //Debug κωδικοποιημένο
          k = 0; //ετοιμασία για το επόμενο
          for (j = 0; j < 16; ++j) //16 φορές
              {
               shift(l, -(pgm_read_byte(&SHIFT1[15-j]))); //Αντίοστροφη ολίσθηση
               xor_tbl(l, keys[15-j]);  //XOR με αντίστροφη σειρά
              }
          permute((byte*) PI2, (byte*) l, (byte*) o);  //Permute με αντίστροφο πίνακα
          o[8] = '\0';
          //Serial.println(o); Debug
          for (j = 0; j < 8; ++j)
               out[idx++] = o[j];
         }
     }
 out[idx] = '\0'; //Τερμάτησε το array ώστε να λειτουργήσει σωστά η removePadding
 removePadding(out);
}

//Κωδικοποίηση μηνύματος και επιστροφή character array σε BASE64
void raw(byte *msg, byte *out)
{
  strcpy((char*)out, (char*)msg);
}
