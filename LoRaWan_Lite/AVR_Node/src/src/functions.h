//Διαβάζει τα 8 bit (5 χρησιμοποιεί) από τον ανεμοδείκτη μέσα από τον καταχωρητή ολίσθησης
byte readHC597()
{
 byte out = 0, i;
 digitalWrite(SCLK_PIN, LOW); //Αρχικά το Clk 0
 digitalWrite(STCP_PIN, LOW); //Αρχικά το Latch 0
 digitalWrite(STCP_PIN, HIGH); //Κάνε Latch
 out |= digitalRead(SDATA_IN_PIN); //Διάβασε το bit 7
 //Για 7 φορές
 for (i=0; i<7; i++)
     {
      out = out << 1; //Ολίσθηση αριστερά κατά 1 bit
      clockPulse(); //Δώσε παλμό στο S.R.
      out |= digitalRead(SDATA_IN_PIN); //Διάβασε με την σειρά τα bit 6, 5, 4, 3, 2, 1, 0
     }
 return out;
}

void writeHC595(byte x)
{
 byte i;
 digitalWrite(P_LOAD, LOW);
 for (i=0; i<8; i++)
     {
      (x & 0b10000000) ? digitalWrite(SDATA_OUT_PIN, HIGH) : digitalWrite(SDATA_OUT_PIN, LOW);
      clockPulse();
      x = x << 1;
     }
 digitalWrite(P_LOAD, HIGH);
}

void ReadSensors()
{
 //char t_stamp[22];
     //------------ Διάβασε τιμή εσωτερικού αισθητήρα ------------------------
     float new_t_int = int_temp.getTempCByIndex(0);
     if (new_t_int == -127) //Αν υπάρχει CRC Error
         Serial.println(F("Failed to read from Internal sensor!"));
     else
        {
         t_int =  new_t_int;
         T_int_S += t_int; //Άθροισμα για υπολογισμό MO κάθε 5 λεπτά
         if (t_int > max_t_int)
            {
             max_t_int = t_int;
             //curDateTime().toCharArray(t_stamp, 22);
             //sprintf(max_t_int_timestamp, "[%s] %2.1f°C", t_stamp, max_t_int);
            }
         //sprintf(t_int_str, "%2.1f", (double) t_int); //Kρατάει μόνο ένα δεκαδικό ψηφίο
         //t_int_str = t_int.c_str();
         

         Serial.println(String(t_int).substring(0,4)); //Debug 
        }
     //ext_temp.requestTemperatures();
     int_temp.requestTemperatures();

      //------------ Διάβασε θερμοκρασία από αισθητήρα DHT22 -------------------
     float newT = dht.readTemperature();
     //Αν απέτυχε το διάβασμα να μην αλλάξει την παλιά τιμή
     if (isnan(newT)) 
         Serial.println(F("Failed to read from DHT sensor!"));    
     else 
        {
         dht_temp = newT;
         dht_temp_S += dht_temp; //Άθροισμα για υπολογισμό MO κάθε 5 λεπτά
         if (dht_temp > max_dht_temp)
            {
             max_dht_temp = dht_temp;
             //curDateTime().toCharArray(t_stamp, 22);
             //sprintf(max_dht_temp_timestamp, "[%s] %2.1f°C", t_stamp, max_dht_temp);
            }
         if (dht_temp < min_dht_temp)
            {
             min_dht_temp = dht_temp;
             //curDateTime().toCharArray(t_stamp, 22);
             //sprintf(min_dht_temp_timestamp, "[%s] %2.1f°C", t_stamp, min_dht_temp);
            }
         //sprintf(dht_temp_str, "%2.1f", dht_temp);
         Serial.println(String(dht_temp).substring(0,4)); //Debug 
        }
     //------------ Διάβασε υγρασία από αισθητήρα DHT22 -----------------------
     float newH = dht.readHumidity();
     //Αν απέτυχε το διάβασμα να μην αλλάξει την παλιά τιμή
     if (isnan(newH)) 
         Serial.println("Failed to read from DHT sensor!"); 
     else 
         {
          humidity = newH;
          humidity_S += humidity; //Άθροισμα για υπολογισμό MO κάθε 5 λεπτά
          if (humidity > max_humidity)
            {
             max_humidity = humidity;
             //curDateTime().toCharArray(t_stamp, 22);
             //sprintf(max_Humidity_timestamp, "[%s] %3.1f", t_stamp, max_humidity);
            }
          if (humidity < min_humidity)
            {
             min_humidity = humidity;
             //curDateTime().toCharArray(t_stamp, 22);
             //sprintf(min_Humidity_timestamp, "[%s] %3.1f", t_stamp, min_humidity);
            }
          //sprintf(hum_str, "%2.1f", humidity); //Δύο ακέραια και ένα δεκαδικό
          //Serial.println(hum_str); //Debug
          Serial.println(String(humidity).substring(0,4)); //Debug 
         }
}

void blink_times_d(byte n, byte duty_cycle, unsigned int period)
{
 unsigned int on_delay, off_delay;
 byte i;
 on_delay = (unsigned long) duty_cycle * period / 100;
 off_delay = period - on_delay;
 for(i = 0; i < n; ++i)
    {
     digitalWrite(STATUS_LED, HIGH);
     delay(on_delay);
     digitalWrite(STATUS_LED, LOW);
     delay(off_delay);
    }
 //wdt_reset();
}

byte blnk_times = 0;
unsigned long blnk_t1 = 0;
bool blnk_state = false;

void blink_nb(byte n, byte duty_cycle, unsigned int period, unsigned int wait_time)
{
 unsigned int on_delay, off_delay;
 on_delay = (unsigned long) duty_cycle * period / 100;
 off_delay = period - on_delay;
 if (blnk_times < n)
    { 
      if (!blnk_state)
         {
          if (millis() - blnk_t1 > off_delay)
             {
              digitalWrite(STATUS_LED, HIGH);
              blnk_state = true;
              blnk_t1 = millis();
             }
         }
      else
         {
          if (millis() - blnk_t1 > on_delay)
             {
              digitalWrite(STATUS_LED, LOW);
              blnk_state = false;
              blnk_t1 = millis();
              ++blnk_times;
             }
         }
    }
 else
    {
     if (millis() - blnk_t1 > wait_time)
        {
         blnk_times = 0;
         if (error_code == TX_ENABLE)
             error_code = error_code_old;
        }
    }
}

void blink_err_code()
{
 switch (error_code)
      {
       case SETUP_ERROR: //Setup Error. Το module δεν επικοινωνεί με το Arduino
          blink_nb(1, 50, 200, 0);
          break;
       case NO_REPLY_ERROR: //Δεν έχει λάβει απάντηση από την άλλη πλευρά τα τελευταία λεπτά
          blink_nb(1, 70, 200, 1000);
          break;
       case TX_ENABLE: //Κάνει εκπομπή
          blink_nb(1, 95, 3000, 100);
          break;
       default: //0 - Υπάρχει επιβεβαίωση από την απέναντι πλευρά για τα τελευταία λεπτά
          blink_nb(3, 30, 200, 2000);
      }
}

void show_dig_blink(byte n)
{
 byte i;
 delay(1000);
 for (i = 0; i < n; ++i)
     {
      blink_times_d(dig[i], 25, 800);
      delay(1000);
     }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for(int i = 0; i <= maxIndex && found <= index; i++)
     {
      if(data.charAt(i) == separator || i == maxIndex)
        {
         found++;
         strIndex[0] = strIndex[1] + 1;
         strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
     }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void print_table(byte *table, byte len, char base)
{
 byte i;
 for(i=0; i < len-1; i++)
    {
     if (base == 'd')
         Serial.print(table[i]);
     else if (base == 'h')
         Serial.print(table[i], HEX);
     Serial.print(':');  
    }
     if (base == 'd')
         Serial.println(table[i]);
     else if (base == 'h')
         Serial.println(table[i], HEX);
}

/*
// Setup the Watch Dog Timer (WDT)
void setupWatchDogTimer() {
  // The MCU Status Register (MCUSR) is used to tell the cause of the last
  // reset, such as brown-out reset, watchdog reset, etc.
  // NOTE: for security reasons, there is a timed sequence for clearing the
  // WDE and changing the time-out configuration. If you don't use this
  // sequence properly, you'll get unexpected results.

  // Clear the reset flag on the MCUSR, the WDRF bit (bit 3).
  MCUSR &= ~(1<<WDRF);

  // Configure the Watchdog timer Control Register (WDTCSR)
  // The WDTCSR is used for configuring the time-out, mode of operation, etc

  // In order to change WDE or the pre-scaler, we need to set WDCE (This will
  // allow updates for 4 clock cycles).

  // Set the WDCE bit (bit 4) and the WDE bit (bit 3) of the WDTCSR. The WDCE
  // bit must be set in order to change WDE or the watchdog pre-scalers.
  // Setting the WDCE bit will allow updates to the pre-scalers and WDE for 4
  // clock cycles then it will be reset by hardware.
  WDTCSR |= (1<<WDCE) | (1<<WDE);
*/
  /**
   *  Setting the watchdog pre-scaler value with VCC = 5.0V and 16mHZ
   *  WDP3 WDP2 WDP1 WDP0 | Number of WDT | Typical Time-out at Oscillator Cycles
   *  0    0    0    0    |   2K cycles   | 16 ms
   *  0    0    0    1    |   4K cycles   | 32 ms
   *  0    0    1    0    |   8K cycles   | 64 ms
   *  0    0    1    1    |  16K cycles   | 0.125 s
   *  0    1    0    0    |  32K cycles   | 0.25 s
   *  0    1    0    1    |  64K cycles   | 0.5 s
   *  0    1    1    0    |  128K cycles  | 1.0 s
   *  0    1    1    1    |  256K cycles  | 2.0 s
   *  1    0    0    0    |  512K cycles  | 4.0 s
   *  1    0    0    1    | 1024K cycles  | 8.0 s
  */
 /*
  WDTCSR  = (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0);  //Κάθε 8 sec
  // Enable the WD interrupt (note: no reset).
  WDTCSR |= _BV(WDIE);
}
*/


/*
//Εισάγει την πλακέτα σε Sleep Mode
void enterSleep(void)
{
  // There are five different sleep modes in order of power saving:
  // SLEEP_MODE_IDLE - the lowest power saving mode
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN - the highest power saving mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //SLEEP_MODE_PWR_DOWN 8mA σε κατάσταση αναμονής
  sleep_enable();
  Serial.end();
  byte old_ADCSRA = ADCSRA;
  ADCSRA = 0; // disable ADC to save power
  // Now enter sleep mode.
  while (!f_wdt) sleep_mode(); //Όσο το f_wdt είναι false να παραμένει στο ίδιο mode //Επίσης το ίδιο γίνεται με sleep_cpu()
  //Η εκτέλεση θα συνεχίσει εδώ μετά το ξύπνημα
  sleep_disable(); //Απενεργοποίηση του sleep
  Serial.begin(115200);
  ADCSRA = old_ADCSRA;
  power_all_enable(); //Ενεργοποίηση όλων των περιφερειακών
}

unsigned int wdt_times = 0;
//Watch Dog Timer ISR. Αυτό γίνεται όταν το WDT υπερχειλίζει δηλ. κάθε 8 sec
ISR (WDT_vect) 
    {
     if (!f_wdt) 
        {
         if (wdt_times < INTERVAL - 1)
             wdt_times++;
         //Βγάλε το Arduino από το Sleep Mode όταν περάσει ο χρόνος INTERVAL * 8sec
         else
             {
              f_wdt = true;
              wdt_times = 0;
             }
        }
    }
*/