#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

//-------------------- Θερμόμετρα One Wire --------------------------
#define OW_INT_PIN           17  //A3
#define RESOLUTION            9  //9, 10, 11, 12 bit
//------------------------ DHT22 ------------------------------------
#define DHTPIN               18 //A4 
#define DHTTYPE              DHT22     //DHT 22 (AM2302)

#define PERIOD                1000 //Περίοδος μετρήσεων

OneWire ow_int(OW_INT_PIN);
DallasTemperature int_temp(&ow_int);
DeviceAddress int_temp_addr;
DHT dht(DHTPIN, DHTTYPE);

char t_ext_str[6], t_int_str[6];
unsigned long lastTempRequest = 0;

void init_thermo()
  {
   int_temp.begin();
   int_temp.getAddress(int_temp_addr, 0);
   int_temp.setResolution(int_temp_addr, RESOLUTION);
   int_temp.setWaitForConversion(false);
   int_temp.requestTemperatures();
   lastTempRequest = millis(); 
  }
