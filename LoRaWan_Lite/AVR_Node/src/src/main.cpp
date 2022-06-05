/*** LoRa WAN Lite Node ***/
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#include "definitions.h"
#include "thermo.h"
#include "functions.h"
#include "thermo_functions.h"
#include "cmd.h"
#include "commands.h"
#include "encrypt.h"
#include "lora.h"
#include "ClassA_Dev.h"
#include "base64.hpp"

//Μήνυμα έως 128 χαρακτήρες //230
//------------          1         2         3         4         5         6         7         8         9        10        11        12        13        14        15        16        17        18        19        20        21        22        23                                                                                                
//------------ 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
//char m[] =  "@*Hello world 12345678 preveza prevezas stavros1234 -----1 sdsdd fgfgfg   rrrtt    ~~~~~~``````m mmm ##### $$$$$$$$$$$ ><><>---."; //Χρήση Character Array
//char m[] =  "Test Preveza: ";
String tmp_str;

void setup() 
{
  cmdInit(115200);
  while (!Serial);        //Περίμενε
  pinMode(FAN_PIN, OUTPUT);
  pinMode(SCLK_PIN, OUTPUT);
  pinMode(STCP_PIN, OUTPUT);
  pinMode(P_LOAD, OUTPUT);
  pinMode(SDATA_IN_PIN, INPUT);
  pinMode(STATUS_LED, OUTPUT); 
  pinMode(LED1, OUTPUT);
  blink_times_d(3, 15, 400);  //Αναβόσβησε 3 φορές γρήγορα
  eeprom_init();
  //Εμφάνισε κατάσταση
  Serial.println(F("***** ETHIC [Stavros S. Fotoglou - 1o EPAL PREVEZAS - EK PREVEZAS] *****"));
  lora_init();
  randomSeed(analogRead(A2));
  //Παραγωγή κλειδιών βάσει του κλειδιού 64bit και αποθήκευση στον καθολικό πίνακα keys 16x8
  keys_generator();
  LoRa.idle();   //Πήγαινε το Modem σε κατάσταση Idle
  init_thermo();  //Αρχικοποίηση θερμομέτρων 1-wire
  dht.begin();    //Αρχικοποίηση DHT22
  dout_status = 0;
  writeHC595(0);  //Κάνε Off όλες τις εξόδους αν έχει σκουπίδια μετά το reset
  init_commands(); //Αρχικοποίησε εντολές για την σειριακή γραμμή εντολών
  get_lora(1, 0);  //Εμφάνισε αρχική κατάσταση του LoRa MoDem
  delay(1000);
  win_speed = 210; //160 - 255 Ταχύτητα PWM για κίνηση παραθύρων
  air_stength = 0; //70 - 200 Ταχύτητα PWM για εξαερισμό
  interval1 = interval2 = interval3 = millis();
  imed_flag = true; //Στην αρχή να διαβάσει αισθητήρες χωρίς να περιμένει και να κάνει άμεσα εκπομπή. Το 1ο μήνυμα είναι χωρίς επιβεβαίωση λήψης
  status_str = "";
  uptime = 0;
  rssi = -160;
  //verb = 1; //Βγάλε σχόλιο αν θέλουμε verbosity 1 ή 2
  wdt_enable(WDTO_8S); //Βάλε το Watch Dog στα 8 sec
}

void loop() 
{
 String moist_str, skia_str, w1stat_str, w2stat_str, heat_str, pump_str, uptime_str;
 //--- Βγάλε σχόλιο παρακάτω ώστε να δέχεται εντολές από σειριακή κονσόλα. Επίσης στην γραμμή 68 ClassA_Dev.h ---
 cmdPoll(); //Έλεγξε για εντολή στην κονσόλα
 act(); //Διάβασε διακόπτες και κίνησε ενεργοποιητές
 if (strlen(instr) > 0)
    {
     Serial.print(F(">>>>>"));
     Serial.println(instr);
     cmd_parse(instr); //Ψάξε για εντολή
     instr[0]='\0';
    }
 //Αν έφτασε ο χρόνος δειγματοληψίας ή απαιτεί άμεση εκπομπή τότε διάβασε τιμές αισθητήρων
 if (((millis() - interval1) > AQUIRE_MS) || imed_flag)
    {
     ReadSensors();
     status_str = "";
     //Εξωτερική Θερμ. ; Εσωτερική Θερμ. ; Υγρασία ; Ph ; Ένταση Ηλιακής ακτιν. ; Στάθμη νερού (U/D = Up/Down) ; Κατάσταση κουρτίνας (O/C/S = Open/Close/Step) ;
     //Κατάσταση παραθύρου 1 (O/C/S) ; Κατάσταση παραθύρου 2 (O/C/S) ; Ταχύτητα παραθύρων ; Ταχύτητα εξαερισμού ; Θέρμανση (O/F = On/ofF) ; Αντλία (O/F)
     /*(din_status & _BV(DIN_MOIST)) ? moist_str = "U" : moist_str = "D"; 
     (!(din_status & _BV(DIN_SKT1))) ? skia_str = "O" : ((!(din_status & _BV(DIN_SKT2))) ? skia_str = "C" : skia_str = "S");
     (!(din_status & _BV(DIN_W1T1))) ? w1stat_str = "O" : ((!(din_status & _BV(DIN_W1T2))) ? w1stat_str = "C" : w1stat_str = "S");
     (!(din_status & _BV(DIN_W2T1))) ? w2stat_str = "O" : ((!(din_status & _BV(DIN_W2T2))) ? w2stat_str = "C" : w2stat_str = "S");
     (dout_status & _BV(DOUT_LAMP)) ? heat_str = "O" : heat_str = "F";
     (dout_status & _BV(DOUT_PUMP)) ? pump_str = "O" : pump_str = "F"; */
     uptime_str = String(uptime);
     status_str = t_ext_str + ";" + t_int_str + ";" + hum_str + ";" + ph_str + ";" + irrad_str + ";" + String(din_status) + ";" + String(dout_status) + ";" +
                  String(win_speed) + ";" + String(air_idx) + ";" + String(rssi) + ";" + uptime_str; //";4294967296"
     /*status_str = t_ext_str + ";" + t_int_str + ";" + hum_str + ";" + ph_str + ";" + irrad_str + ";" + moist_str + ";" + skia_str + ";" + 
                  w1stat_str + ";" + w2stat_str + ";" + String(win_speed) + ";" + String(air_idx) + ";" + heat_str + ";" + pump_str + ";" + 
                  String(rssi) + ";" + uptime_str; //";4294967296"*/
     if (verb == 2)
         Serial.println(status_str); //Debug
     interval1 = millis();
    }

 //Αν πέρασε ο χρόνος ή απαιτεί άμεση εκπομπή τότε κάνε προετοιμασία για εκπομπή πακέτου
 if (((millis() - interval2) > LORA_PREP_MS) || imed_flag)
    {
     //tmp_str = m;
     tmp_str = status_str;
     payload_len = encrypt((byte*)tmp_str.c_str(), (byte*)out); //Κρυπτογράφησε με DES
     //raw(m, out); //ή ετοίμασε μη κρυπτογραφημένα δεδομένα
     //Serial.println(out); //Debug
     //Serial.println(yy); //Debug   
     //decrypt((byte*)out, oo, yy); //Debug για δοκιμή αλγορίθμου
     interval2 = millis();
    }
 
 //Αν πέρασε ο χρόνος ή απαιτεί άμεση εκπομπή τότε κάνε εκπομπή πακέτου
 if (((millis() - interval3) > LORA_TX_MS) || imed_flag)
    {
     time_4_tx = true;
     //Αν απαιτηθεί άμεση εκπομπή μετά από αλλαγή κάποιου ενεργοποιητή τότε να μη ζητήσει επιβεβαίωση λήψης
     if (imed_flag)
        {
         confRqst = UNCONFIRM; //CONFIRM_REQ ή UNCONFIRM;
         imed_flag = false;
        }
     else
         confRqst = CONFIRM_REQ; //Αν εκπέμπει περιοδικά την κατάσταση όταν έρθει ο χρόνος, να ζητάει επιβεβαίωση λήψης από το δίκτυο
     interval3 = interval2 = interval1 = millis();
     uptime += UPTIME_SEC;
    }

 //Αν είναι ενεργοποιημένο κάνε εκπομπή του πακέτου και περίμενε για λήψη. Εκτελείται συνέχεια
 if (time_4_tx)
    {
     //Κάνε εκπομπή και άνοιξε τα παράθυρα λήψης
     ClassA_Dev();
     //Αν έχει ληφθεί κάποιο frame στα παράθυρα λήψης, τότε συνέχισε με έλεγχο και επεξεργασία
     if (rx_state == RX_DONE)
         handleRxFrame();
    }
 //Αλλιώς αν τελείωσε ο προηγούμενος κύκλος εκπομπής - λήψης
 else
    {
     //Αν έχει λάβει έγκυρο option και θέλει να ανοίξει αμέσως ένα νέο κύκλο Tx
     if (optOK)
        {
         optOK = false; //Μην ξαναμπείς εδώ μέχρι να λάβει νέα εντολή MAC
         imed_flag = true; //Στείλε τώρα
        }
     //Αν είναι ενεργοποιημένο τότε άλλαξε συχνότητα ή DR   
     if (updateLater == 2)
        {
         updateLoRa();
         updateLater = 0;
        }
    }
 
 wdt_reset(); //Σε περίπτωση που κολλήσει να κάνει reset
}