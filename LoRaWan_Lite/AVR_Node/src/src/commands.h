void updateLoRa(void);
void setup_defaults(void);
void init_commands(void);
void printErr1(void);
void printErr2(void);
void setwspeed(int, char**);
void win_op(int, char**);
void win_cl(int, char**);
void win_fop(int, char**);
void win_fcl(int, char**);
void skia_op(int, char**);
void skia_cl(int, char**);
void skia_fop(int, char**);
void skia_fcl(int, char**);
void air_speed(int, char**);
void get_status(int, char**);
void set_heater(int, char**);
void set_pump(int, char**);
void print_version(int, char**);
void set_lora(int, char**);
void get_lora(int, char**);
void ee_clear(int, char**);
void set_verb(int, char**);
void ee_save(int, char**);
void soft_reset(int, char**);
void help(int, char**);

const char PROGMEM \
   //        1         |           2         |         3            |         4            |          5          |
   ct01[] = "setwspeed", ct02[] = "winop"    , ct03[] = "wincl"     , ct04[] = "winfop"    , ct05[] = "winfcl"   ,
   ct06[] = "skiaop"   , ct07[] = "skiacl"   , ct08[] = "skiafop"   , ct09[] = "skiafcl"   , ct10[] = "airspeed" ,
   ct11[] = "getstatus", ct12[] = "heater"   , ct13[] = "pump"      , ct14[] = "version"   , ct15[] = "setlora"  ,
   ct16[] = "getlora"  , ct17[] = "eeclear"  , ct18[] = "verb"      , ct19[] = "save"      , ct20[] = "rst"      ,
   ct21[] = "?";

//Δομή συνδεδεμένης λίστας
const L_cmd PROGMEM cmd21 = {(char*) ct21   , help,             NULL}; //? Παρουσίασε εντολές
const L_cmd PROGMEM cmd20 = {(char*) ct20   , soft_reset,     &cmd21}; //Κάνει reset
const L_cmd PROGMEM cmd19 = {(char*) ct19   , ee_save,        &cmd20}; //Αποθηκεύει ρυθμίσεις στην EEPROM
const L_cmd PROGMEM cmd18 = {(char*) ct18   , set_verb,       &cmd19}; //Θέτει το verbosity σε 0, 1 ή 2
const L_cmd PROGMEM cmd17 = {(char*) ct17   , ee_clear,       &cmd18}; //Καθάρισε eeprom και βάλε default τιμές
const L_cmd PROGMEM cmd16 = {(char*) ct16   , get_lora,       &cmd17}; //Εμφάνιση παραμέτρων Lora frequency, sf, bw, cr, power
const L_cmd PROGMEM cmd15 = {(char*) ct15   , set_lora,       &cmd16}; //Ρύθμιση παραμέτρων Lora frequency, sf, bw, cr, power
const L_cmd PROGMEM cmd14 = {(char*) ct14   , print_version,  &cmd15}; //Έκδοση λογισμικού
const L_cmd PROGMEM cmd13 = {(char*) ct13   , set_pump,       &cmd14}; //Αντλία on ή off
const L_cmd PROGMEM cmd12 = {(char*) ct12   , set_heater,     &cmd13}; //Θέρμανση on ή off
const L_cmd PROGMEM cmd11 = {(char*) ct11   , get_status,     &cmd12}; //Διάβασε κατάσταση αισθητήρων
const L_cmd PROGMEM cmd10 = {(char*) ct10   , air_speed,      &cmd11}; //Ταχύτητα εξαερισμού 0: σταμάτα, 60-250
const L_cmd PROGMEM cmd09 = {(char*) ct09   , skia_fcl,       &cmd10}; //Κλείσε σκιά πλήρως - Άπλωσε
const L_cmd PROGMEM cmd08 = {(char*) ct08   , skia_fop,       &cmd09}; //Άνοιξε σκιά πλήρως - Μάζεψε
const L_cmd PROGMEM cmd07 = {(char*) ct07   , skia_cl,        &cmd08}; //Κλείσε σκιά με βήμα-χρόνο
const L_cmd PROGMEM cmd06 = {(char*) ct06   , skia_op,        &cmd07}; //Άνοιξε σκιά με βήμα-χρόνο
const L_cmd PROGMEM cmd05 = {(char*) ct05   , win_fcl,        &cmd06}; //Κλείσε παράθυρο πλήρως
const L_cmd PROGMEM cmd04 = {(char*) ct04   , win_fop,        &cmd05}; //Άνοιξε παράθυρο πλήρως
const L_cmd PROGMEM cmd03 = {(char*) ct03   , win_cl,         &cmd04}; //Κλείσε παράθυρο με βήμα-χρόνο
const L_cmd PROGMEM cmd02 = {(char*) ct02   , win_op,         &cmd03}; //Άνοιξε παράθυρο με βήμα-χρόνο
const L_cmd PROGMEM cmd01 = {(char*) ct01   , setwspeed,      &cmd02}; //Θέσε ταχύτητα παραθύρων

void init_commands(void)
{
 cmd_tbl = (L_cmd*) &cmd01;
}

void printErr1()
{
 Serial.println(F("Too few arguments"));
}

void printErr2()
{
 Serial.println(F("Syntax error"));
}

//Αλλάζει τις ζητούμενες στροφές του κινητήρα
void setwspeed(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     win_speed = atoi(args[1]);
    }
 else
     printErr1();
}

//Ανοίγει παράθυρα για κάποιο χρόνο
void win_op(int arg_cnt, char **args)
{
 byte w;
 unsigned int t;
 if (arg_cnt > 2)
    {
     w = atoi(args[1]);
     t = atoi(args[2]);
     if (w == 1)
         win1_open(t);
     else if (w == 2)
         win2_open(t);
    }
 else
     printErr1();
}

//Κλείνει παράθυρα για κάποιο χρόνο
void win_cl(int arg_cnt, char **args)
{
 byte w;
 unsigned int t;
 if (arg_cnt > 2)
    {
     w = atoi(args[1]);
     t = atoi(args[2]);
     if (w == 1)
         win1_close(t);
     else if (w == 2)
         win2_close(t);
    }
 else
     printErr1();
}

//Ανοίγει πλήρως παράθυρα
void win_fop(int arg_cnt, char **args)
{
 byte w;
 if (arg_cnt > 1)
    {
     w = atoi(args[1]);
     if (w == 1)
         win1_open(MAX_WIN_TIME);
     else if (w == 2)
         win2_open(MAX_WIN_TIME);
    }
 else
     printErr1();
}

//Κλείνει πλήρως παράθυρα
void win_fcl(int arg_cnt, char **args)
{
 byte w;
 if (arg_cnt > 1)
    {
     w = atoi(args[1]);
     if (w == 1)
         win1_close(MAX_WIN_TIME);
     else if (w == 2)
         win2_close(MAX_WIN_TIME);
    }
 else
     printErr1();
}

//Μαζεύει την σκίαση για κάποιο χρόνο
void skia_op(int arg_cnt, char **args)
{
 unsigned int t;
 if (arg_cnt > 1)
    {
     t = atoi(args[1]);
     skia_open(t);
    }
 else
     printErr1();
}

//Απλώνει την σκίαση για κάποιο χρόνο
void skia_cl(int arg_cnt, char **args)
{
 unsigned int t;
 if (arg_cnt > 1)
    {
     t = atoi(args[1]);
     skia_close(t);
    }
 else
     printErr1();
}

//Μαζεύει πλήρως την σκίαση 
void skia_fop(int arg_cnt, char **args)
{
 skia_open(MAX_SKIA_TIME);
}

//Απλώνει πλήρως την σκίαση 
void skia_fcl(int arg_cnt, char **args)
{
 skia_close(MAX_SKIA_TIME);
}

void print_version(int arg_cnt, char **args)
{
 Serial.print(F("Version: "));
 Serial.println(VERSION);
}

void air_speed(int arg_cnt, char **args)
{
 unsigned int t;
 if (arg_cnt > 1)
    {
     t = atoi(args[1]);
     if (t >= 0 && t <= 5)
        {
         if (air_idx != t)
            {
             air_idx = t;
             air_stength = get_air_speed(air_idx); //0-5
             analogWrite(FAN_PIN, air_stength); //70 - 200
             optOK = true;
            }
        }
    }
 else
     printErr1();
}

void get_status(int arg_cnt, char **args)
{
 Serial.println(status_str);
}

void set_heater(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     if (strcmp_P(args[1], PSTR("on")) == 0)
        { LAMP_ON(); }
     else if (strcmp_P(args[1], PSTR("off")) == 0)
        { LAMP_OFF(); }
     else printErr2(); //Syntax error
    }
 else
     printErr1();
}

void set_pump(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     if (strcmp_P(args[1], PSTR("on")) == 0)
        { PUMP_ON(); 
          potisma_time = millis();
        }
     else if (strcmp_P(args[1], PSTR("off")) == 0)
        { PUMP_OFF(); }
     else printErr2(); //Syntax error
    }
 else
     printErr1();
}

void set_lora(int arg_cnt, char **args)
{
 if (arg_cnt > 5)
    {
     frequency = atol(args[1]);
     sf = atoi(args[2]);
     bw = atol(args[3]);
     cr = atoi(args[4]);
     power = atoi(args[5]);
     updateLoRa();
    }
 else
     printErr1();
}

void get_lora(int arg_cnt, char **args)
{
 char buf[48];
 sprintf(buf, "Freq:%ld SF:%d BW:%ld CR:4/%d Pr:%d", frequency, sf, bw, cr, power);
 Serial.println(buf);
}

void ee_clear(int arg_cnt, char **args)
{
 setup_defaults();
 delay(500);
 resetFunc();
}

void set_verb(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     verb = atoi(args[1]);
     Serial.print(F("Set verbosity to "));
     Serial.println(verb);
    }
 else
     printErr1();
}

void ee_save(int arg_cnt, char **args)
{
 EEPROM.put(FREQUENCY_ADDR, (long) frequency);
 EEPROM.put(SF_ADDR, (byte) sf);
 EEPROM.put(BW_ADDR, (long) bw);
 EEPROM.put(CR_ADDR, (byte) cr);
 EEPROM.put(POWER_ADDR, (byte) power);
 Serial.println(F("Save OK"));
}

void soft_reset(int arg_cnt, char **args)
{
 resetFunc();
}

void help(int arg_cnt, char **args)
{
 char buf[10];
 L_cmd *cmd_entry;
 for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = (L_cmd*)pgm_read_word(&cmd_entry->next))
     {
      strcpy_P(buf, (char*)pgm_read_word(&cmd_entry->cmd));
      Serial.println(buf);
     }
}
