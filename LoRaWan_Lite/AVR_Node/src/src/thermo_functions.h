
unsigned long win1_time_counter, win2_time_counter, skia_time_counter, potisma_time;

//Ανοίγει το παράθυρο 1
void win1_open(unsigned int motion_time)
{
  W1_OPEN();
  win1_time_counter = millis() + motion_time;
  analogWrite(WINDOW_SPEED_PIN, win_speed);
}

//Κλείνει το παράθυρο 1
void win1_close(unsigned int motion_time)
{
  W1_CLOSE();
  win1_time_counter = millis() + motion_time;
  analogWrite(WINDOW_SPEED_PIN, win_speed);
}

//Ανοίγει το παράθυρο 2
void win2_open(unsigned int motion_time)
{
  W2_OPEN();
  win2_time_counter = millis() + motion_time;
  analogWrite(WINDOW_SPEED_PIN, win_speed);
}

//Κλείνει το παράθυρο 2
void win2_close(unsigned int motion_time)
{
  W2_CLOSE();
  win2_time_counter = millis() + motion_time;
  analogWrite(WINDOW_SPEED_PIN, win_speed);
}

//Μαζεύει την σκίαση
void skia_open(unsigned int motion_time)
{
  SKIA_OPEN();
  skia_time_counter = millis() + motion_time;
}

//Απλώνει την σκίαση
void skia_close(unsigned int motion_time)
{
  SKIA_CLOSE();
  skia_time_counter = millis() + motion_time;
}

//Η διαδικασία καλείται συνέχεια από την Loop
void act()
{
 //Διάβασε την κατάσταση των τερματικών διακοπτών
 din_status = readHC597();
 //===== Παράθυρο 1 =====
 if ((dout_status & _BV(DOUT_W1_C2)) || (dout_status & _BV(DOUT_W1_C1))) //Αν έχει δοθεί εντολή σε μοτερ για άνοιγμα ή κλείσιμο
     if (\
         ((dout_status & _BV(DOUT_W1_C2)) && !(din_status & _BV(DIN_W1T1))) ||  //Αν δόθηκε άνοιγμα και ο τερματικός opened (T1) έκλεισε Ή 
         ((dout_status & _BV(DOUT_W1_C1)) && !(din_status & _BV(DIN_W1T2))) ||  //Δόθηκε κλείσιμο και ο τερματικός closed (T2) έκλεισε   Ή
         (millis() >= win1_time_counter) //Πέρασε ο χρόνος για άνοιγμα ή κλείσιμο ΤΟΤΕ
        )
        {
         W1_STOP();  //Σταμάτα την παροχή από half bridge
         analogWrite(WINDOW_SPEED_PIN, 0); //Σταμάτα το PWM
        }
  //===== Παράθυρο 2 =====
  if ((dout_status & _BV(DOUT_W2_C2)) || (dout_status & _BV(DOUT_W2_C1))) //Αν έχει δοθεί εντολή σε μοτερ για άνοιγμα ή κλείσιμο
      if (\
          ((dout_status & _BV(DOUT_W2_C2)) && !(din_status & _BV(DIN_W2T1))) ||  //Αν δόθηκε άνοιγμα και ο τερματικός opened (T1) έκλεισε Ή 
          ((dout_status & _BV(DOUT_W2_C1)) && !(din_status & _BV(DIN_W2T2))) ||  //Δόθηκε κλείσιμο και ο τερματικός closed (T2) έκλεισε   Ή
          (millis() >= win2_time_counter) //Πέρασε ο χρόνος για άνοιγμα ή κλείσιμο ΤΟΤΕ
         )
         {
          W2_STOP(); //Σταμάτα την παροχή από half bridge
          analogWrite(WINDOW_SPEED_PIN, 0); //Σταμάτα το PWM
         }
  //===== Σκίαση =====
  if ((dout_status & _BV(DOUT_SK_C2)) || (dout_status & _BV(DOUT_SK_C1))) //Αν έχει δοθεί εντολή σε μοτερ για άνοιγμα ή κλείσιμο
      if (\
          ((dout_status & _BV(DOUT_SK_C2)) && !(din_status & _BV(DIN_SKT1))) ||  //Αν δόθηκε άνοιγμα και ο τερματικός opened (T1) έκλεισε Ή 
          ((dout_status & _BV(DOUT_SK_C1)) && !(din_status & _BV(DIN_SKT2))) ||  //Δόθηκε κλείσιμο και ο τερματικός closed (T2) έκλεισε   Ή
          (millis() >= skia_time_counter) //Πέρασε ο χρόνος για άνοιγμα ή κλείσιμο ΤΟΤΕ
         )
         {
          SKIA_STOP(); //Σταμάτα την παροχή από half bridge
         }
  //===== Πότισμα =====
  if (dout_status & _BV(DOUT_PUMP)) //Αν έχει δοθεί εντολή ποτίσματος
      if ((din_status & _BV(DIN_MOIST)) || ((millis() - potisma_time) >= MAX_POTISMA_TIME)) //Αν η στάθμη έφτασε στο επιθυμητό επίπεδο ή πέρασε ο χρόνος παροχής νερού
         {
          PUMP_OFF(); //Σταμάτα το πότισμα
         }
  //===== Θέρμανση =====
  if (dout_status & _BV(DOUT_LAMP)) //Αν έχει δοθεί εντολή Θέρμανσης
      if (dht_temp > MAX_INT_TEMP)
         {
          LAMP_OFF();
         }
  
 //Ενημέρωσε κατάσταση των εξόδων του S.R.
 writeHC595(dout_status);

 //Αν έγινε κάποια αλλαγή στη κατάσταση ενεργοποιητών να κάνει άμεσα εκπομπή
 if ((dout_status ^ old_dout_status) > 0)
    {
      old_dout_status = dout_status;
      //imed_flag = true;
      optOK = true;
    }

 //Αν έγινε κάποια αλλαγή στη κατάσταση ψηφιακών εισόδων να κάνει άμεσα εκπομπή
 //else if ((din_status ^ old_din_status) > 0)
 //   {
 //     old_din_status = din_status;
 //     imed_flag = true;
 //   }
}