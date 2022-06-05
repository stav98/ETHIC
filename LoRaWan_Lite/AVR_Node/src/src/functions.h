//Κάνει software Reset τον AVR
void(* resetFunc)(void) = 0;

//Διαβάζει τα 8 bit μέσα από τον καταχωρητή ολίσθησης Parallel In to Serial Out
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

//Βγάζει ένα byte στον καταχωρητή ολίσθησης Serail In to Parallel Out
void writeHC595(byte x)
{
 byte i;
 digitalWrite(P_LOAD, LOW); //Προετοίμασε το σήμα Parallel Load
 for (i=0; i<8; i++)
     {
      //Έλεγξε το b7. Αν είναι ένα βγάλε 1 στην σειριακή έξοδο, διαφορετικά 0
      (x & 0b10000000) ? digitalWrite(SDATA_OUT_PIN, HIGH) : digitalWrite(SDATA_OUT_PIN, LOW);
      clockPulse(); //Δώσε ένα παλμό ρολογιού ώστε να ολισθήσουν
      x = x << 1; //Ολίσθησε την μεταβλητή αριστερά ώστε να στείλει το επόμενο bit
     }
 //Στο τέλος στον S.R. υπάρχουν και τα 8 bits στις εξόδους Q0-Q7 
 digitalWrite(P_LOAD, HIGH); //Κάνε latch τα bit που βρίσκονται στα Q0-Q7 του S.R. - Λειτουργεί με ανοδικό μέτωπο παλμού
}

void ReadSensors()
{
     //------------ Διάβασε τιμή εξωτερικού αισθητήρα ------------------------
     float new_t_ext = ext_temp.getTempCByIndex(0);
     if (new_t_ext == -127) //Αν υπάρχει CRC Error
         Serial.println(F("Failed to read from Internal sensor!"));
     else
        {
         t_ext =  new_t_ext;
         T_ext_S += t_ext; //Άθροισμα για υπολογισμό MO κάθε 5 λεπτά
         if (t_ext > max_t_ext)
            {
             max_t_ext = t_ext;
            }
         t_ext_str = String(t_ext).substring(0,4);
         //Serial.println(t_ext_str); //Debug 
        }
     ext_temp.requestTemperatures();

      //------------ Διάβασε θερμοκρασία από εσωτερικό αισθητήρα DHT22 -------------------
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
            }
         if (dht_temp < min_dht_temp)
            {
             min_dht_temp = dht_temp;
            }
         t_int_str = String(dht_temp).substring(0,4);
         //Serial.println(t_int_str); //Debug 
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
            }
          if (humidity < min_humidity)
            {
             min_humidity = humidity;
            }
          hum_str = String(humidity).substring(0,4);
          //Serial.println(hum_str); //Debug 
         }
     //-------------- Διάβασε Ph ----------------------------------------------
     ph = ((882 - analogRead(PH_PIN)) * 0.012) + 2.5; //ph
     ph_str = String(ph).substring(0,4);
     //Serial.println(ph_str); //Debug 

     //-------------- Διάβασε Irradiation --------------------------------------
     irrad = analogRead(IRRAD_PIN);
     irrad_str = String(irrad).substring(0,4);
     //Serial.println(irrad_str); //Debug 
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

//Μετατρέπει τις κλίμακες 0-5 σε ταχύτητα PWM των ανεμιστήρων
byte get_air_speed(byte i)
{
 byte vals[] = {0, 70, 80, 90, 100, 200};
 return vals[i];
}