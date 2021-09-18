
void ClassA_Dev()
{
 if (!send_flag)
    {
     digitalWrite(STATUS_LED, HIGH);
     Serial.print(F("Tx Frame ["));
     Serial.print((Retr_count + 1));
     Serial.println(F("] >>"));
     send_pkt(out, payload_len); //Στείλε το πακέτο
     send_flag = true;
     randDelay = random(100);
    }
 //1ο Rx παράθυρο
 else if ((micros() - rx_time >= (RX1_TIME - RX1_OPEN_TOLLER)) && (rx_state == WAIT_FOR_RX1)) //Θα μπω 100μs νωρίτερα γιατί αργούν οι δύο παρακάτω εντολές
    {
     LoRa.enableInvertIQ(); //Αν είναι Rx window τότε αντιστροφή των I/Q γιατί η πύλη τα αντιστρέφει ώστε να μην ακούει η μια πύλη την άλλη
     LoRa.receive_single();  //Άνοιγμα δέκτη
     d = micros() - rx_time;
     Serial.print(F("Rx1 Window open at: ")); //Debug
     Serial.println(d); //Debug
     rx_state = RX1_WIND_OPEN;
    }
 //2ο Rx παράθυρο - Ανοίγει μόνο όταν δεν λάβει δεδομένα στο παράθυρο Rx-1
 //Το 220 που αφαιρώ είναι ο χρόνος εκτέλεσης της εντολής LoRa.setSpreadingFactor(9)
 else if ((micros() - rx_time >= RX2_TIME - RX1_OPEN_TOLLER - 220) && (rx_state == RX1_WIND_OPEN)) //Θα μπω 100μs νωρίτερα γιατί αργούν οι δύο παρακάτω εντολές
    {
     LoRa.setSpreadingFactor(12); //Βάλε SF 9
     LoRa.enableInvertIQ(); //Αν είναι Rx window τότε αντιστροφή των I/Q γιατί η πύλη τα αντιστρέφει ώστε να μην ακούει η μια πύλη την άλλη
     LoRa.receive_single();  //Άνοιγμα δέκτη
     d = micros() - rx_time;
     Serial.print(F("Rx2 Window open at: ")); //Debug
     Serial.println(d); //Debug
     rx_state = RX2_WIND_OPEN;
    }
        
 //Ανοίξαν και τα δύο παράθυρα RX1 και RX2 με TimeOut ή έλαβε κάποιο πλαίσιο σε ένα από τα δύο παράθυρα
 if (cont > 1)
    {
     cont = 0; //Για επόμενο κύκλο
     LoRa.setSpreadingFactor(SF); //Γύρισε στο προκαθορισμένο SF
     LoRa.disableInvertIQ(); //Αν τελείωσε το παράθυρο λήψης τότε γυρίζουν κανονικά τα I/Q
     digitalWrite(STATUS_LED, LOW); //Σβήσε το LED
     send_flag = false; //Για την επόμενη εκπομπή
     if (!Ack_OK && Retr_count < (TX_RETRIES-1)) //Αν δεν έλαβε ACK και δεν έχει κάνει όλες τις προσπάθειες
        {
         Retr_count++; 
         delay(randDelay + 20); //Τυχαία καθυστέρηση για επανεκπομπή
        }
     //Εδώ έλαβε ACK ή προσπάθησε N φορές
     else
        {
         Fcnt++; //Αύξησε τον μετρητή frame
         Retr_count = 0; //Μηδένισε για την επόμενη φορά
         Ack_OK = false; //Για επόμενο έλεγχο
         sample_flag = true; //Μπορεί να πάρει νέα μέτρηση και να ετοιμάσει το επόμενο πακέτο
         f_wdt = false; //Να μην ξαναμπεί εδώ μέχρι να κάνει διακοπή το Wachdog
         enterSleep(); //Πέσε σε Sleep
        }
    }
}