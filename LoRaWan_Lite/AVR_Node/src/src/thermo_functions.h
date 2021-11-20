
unsigned long win1_time_counter, win2_time_counter, skia_time_counter;

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
 byte din_status = readHC597();
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
 //Ενημέρωσε κατάσταση των εξόδων του S.R.
 writeHC595(dout_status);
}