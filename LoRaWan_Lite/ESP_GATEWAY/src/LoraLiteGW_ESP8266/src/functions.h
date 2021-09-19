byte search_SF(String s)
{
 String datr[] = {"SF7BW125", "SF8BW125", "SF9BW125", "SF10BW125", "SF11BW125", "SF12BW125"};
 byte SF[] = {7, 8, 9, 10, 11, 12};
 byte i = 0;
 for (i = 0; i < 6; i++)
     {
      if (s == datr[i])
          return SF[i];
     }
 return 0;
}

void print_table(byte *table, unsigned short len, char base)
{
 unsigned short i;
 for(i = 0; i < len-1; i++)
    {
     if (base != 'c')
        {
         if (base == 'd')
             Serial.print(table[i]);
         else if (base == 'h')
             Serial.printf("%02X", table[i]);
         Serial.print(':'); 
        }
     else
         Serial.print((char)table[i]);
    }
 if (base != 'c')
    {
     if (base == 'd')
         Serial.println(table[i]);
     else if (base == 'h')
         Serial.printf("%02X\n", table[i]);
    }
 else
    Serial.println((char)table[i]);
}


void blink_led()
{
 if (millis() - blink_per > 400)
    {
     digitalWrite(2, LOW);
     blink_per = millis();
    }
 else if (millis() - blink_per > 200)
      digitalWrite(2, HIGH);
}
