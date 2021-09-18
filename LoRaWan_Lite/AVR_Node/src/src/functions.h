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
 wdt_reset();
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
  WDTCSR  = (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0);  //Κάθε 8 sec
  // Enable the WD interrupt (note: no reset).
  WDTCSR |= _BV(WDIE);
}

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