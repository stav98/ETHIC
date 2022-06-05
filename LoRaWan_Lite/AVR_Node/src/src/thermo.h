#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

//-------------------- Θερμόμετρα One Wire --------------------------
#define OW_EXT_PIN           17  //A3
#define RESOLUTION            9  //9, 10, 11, 12 bit
//------------------------ DHT22 ------------------------------------
#define DHTPIN               18 //A4 
#define DHTTYPE              DHT22     //DHT 22 (AM2302)

#define PERIOD                1000 //Περίοδος μετρήσεων

OneWire ow_ext(OW_EXT_PIN);
DallasTemperature ext_temp(&ow_ext);
DeviceAddress ext_temp_addr;
DHT dht(DHTPIN, DHTTYPE);

//char t_ext_str[6], t_int_str[6];

unsigned long lastTempRequest = 0;

void init_thermo()
  {
   ext_temp.begin();
   ext_temp.getAddress(ext_temp_addr, 0);
   ext_temp.setResolution(ext_temp_addr, RESOLUTION);
   ext_temp.setWaitForConversion(false);
   ext_temp.requestTemperatures();
   lastTempRequest = millis(); 
  }
