#include <SPI.h>

#define LORA_DEFAULT_SPI           SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6 
#define LORA_DEFAULT_SS_PIN        10
#define LORA_DEFAULT_RESET_PIN     9
#define LORA_DEFAULT_DIO0_PIN      2
#define LORA_DEFAULT_DIO1_PIN      3

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1

// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e //Τα δύο λιγότερο σημαντικά είναι το Timeout MSB - default = 00
#define REG_SYMB_TIMEOUT_LSB     0x1f //Εδώ είναι το LSB - Συνολικό μέγεθος 10bit - default = 0x64
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2a
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3b
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06

// PA config
#define PA_BOOST                 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40
#define IRQ_RX_TIME_OUT_MASK       0x80

#define MAX_PKT_LENGTH           255

#if (ESP8266 || ESP32)
    #define ISR_PREFIX ICACHE_RAM_ATTR
#else
    #define ISR_PREFIX
#endif


class LoRaClass : public Stream 
 {
  //------------------------------------------------------------ PRIVATE ----------------------------------------------------------------------------
  private:
    //------ Ιδιότητες ----------------
    SPISettings _spiSettings;
    SPIClass* _spi;
    int _ss;
    int _reset;
    int _dio0;
    int _dio1;
    long _frequency;
    int _packetIndex;
    int _implicitHeaderMode;
    
    void (*_onReceive)(int);
    void (*_onTxDone)();
    void (*_onRxTimeOut)();
    
    //-------- Μέθοδοι -----------------
    void explicitHeaderMode()
          {
           _implicitHeaderMode = 0;
           writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);
          }

    void implicitHeaderMode()
          {
           _implicitHeaderMode = 1;
           writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);
          }

    void handleDio0Rise()
          {
           int irqFlags = readRegister(REG_IRQ_FLAGS);  //Διάβασε το κωδικό διακοπής
           // clear IRQ's
           writeRegister(REG_IRQ_FLAGS, irqFlags); //Αν γράψει πίσω το ίδιο καθαρίζει το IRQ
           if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) //Δεν έγινε λάθος CRC
              {
               if ((irqFlags & IRQ_RX_DONE_MASK) != 0)  //Τελείωσε η λήψη
                  {
                   // received a packet
                   _packetIndex = 0;
                   // read packet length
                   int packetLength = _implicitHeaderMode ? readRegister(REG_PAYLOAD_LENGTH) : readRegister(REG_RX_NB_BYTES);
                   // set FIFO address to current RX address
                   writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));
                   if (_onReceive)
                       _onReceive(packetLength);
      
                  }
               else if ((irqFlags & IRQ_TX_DONE_MASK) != 0) //Τελείωσε η εκπομπή
                  {
                   if (_onTxDone) 
                       _onTxDone();
                  }
              }
          }

    void handleDio1Rise()
          {
           int irqFlags = readRegister(REG_IRQ_FLAGS);  //Διάβασε το κωδικό διακοπής
           // clear IRQ's
           writeRegister(REG_IRQ_FLAGS, irqFlags); //Αν γράψει πίσω το ίδιο καθαρίζει το IRQ
           if ((irqFlags & IRQ_RX_TIME_OUT_MASK) != 0)  //Τελείωσε το παράθυρο λήψης
              {
               if (_onRxTimeOut)
                   _onRxTimeOut();
              }
          }
    
    bool isTransmitting()
          {
           if ((readRegister(REG_OP_MODE) & MODE_TX) == MODE_TX)
               return true;
           if (readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK)
               // clear IRQ's
               writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
           return false;
          }

    int getSpreadingFactor()
          {
           return readRegister(REG_MODEM_CONFIG_2) >> 4;
          }

    long getSignalBandwidth()
          {
           byte bw = (readRegister(REG_MODEM_CONFIG_1) >> 4);
           switch (bw) 
                  {
                   case 0: return 7.8E3;
                   case 1: return 10.4E3;
                   case 2: return 15.6E3;
                   case 3: return 20.8E3;
                   case 4: return 31.25E3;
                   case 5: return 41.7E3;
                   case 6: return 62.5E3;
                   case 7: return 125E3;
                   case 8: return 250E3;
                   case 9: return 500E3;
                  }
           return -1;
          }

    void setLdoFlag()
          {
           // Section 4.1.1.5
           long symbolDuration = 1000 / ( getSignalBandwidth() / (1L << getSpreadingFactor()) ) ;

           // Section 4.1.1.6
           boolean ldoOn = symbolDuration > 16;

           uint8_t config3 = readRegister(REG_MODEM_CONFIG_3);
           bitWrite(config3, 3, ldoOn);
           writeRegister(REG_MODEM_CONFIG_3, config3);
          }

    uint8_t readRegister(uint8_t address)
          {
           return singleTransfer(address & 0x7f, 0x00);
          }

    void writeRegister(uint8_t address, uint8_t value)
          {
           singleTransfer(address | 0x80, value);
          }

    uint8_t singleTransfer(uint8_t address, uint8_t value)
          {
           uint8_t response;
           digitalWrite(_ss, LOW);
           _spi->beginTransaction(_spiSettings);
           _spi->transfer(address);
           response = _spi->transfer(value);
           _spi->endTransaction();
           digitalWrite(_ss, HIGH);
           return response;
          }

    static void onDio0Rise();
    static void onDio1Rise();
    
  //----------------------------------------------------------------------- PUBLIC -------------------------------------------------------------
  public:
    //Κατασκευαστής
    LoRaClass() :
      _spiSettings(LORA_DEFAULT_SPI_FREQUENCY, MSBFIRST, SPI_MODE0),
      _spi(&LORA_DEFAULT_SPI),
      _ss(LORA_DEFAULT_SS_PIN), _reset(LORA_DEFAULT_RESET_PIN), _dio0(LORA_DEFAULT_DIO0_PIN), _dio1(LORA_DEFAULT_DIO1_PIN),
      _frequency(0),
      _packetIndex(0),
      _implicitHeaderMode(0),
      _onReceive(NULL),
      _onTxDone(NULL)
          {
           // overide Stream timeout value
           setTimeout(0);
          }

    int begin(long frequency)
          {
           // setup pins
           pinMode(_ss, OUTPUT);
           // set SS high
           digitalWrite(_ss, HIGH);
           if (_reset != -1) 
              {
               pinMode(_reset, OUTPUT);
               // perform reset
               digitalWrite(_reset, LOW);
               delay(10);
               digitalWrite(_reset, HIGH);
               delay(10);
              }
           // start SPI
           _spi -> begin();
           // check version
           uint8_t version = readRegister(REG_VERSION);
           if (version != 0x12) 
               return 0;
           
           // put in sleep mode
           sleep();
           
           // set frequency
           setFrequency(frequency);

           // set base addresses
           writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
           writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

           // set LNA boost
           writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

           // set auto AGC
           writeRegister(REG_MODEM_CONFIG_3, 0x04);

           // set output power to 17 dBm
           setTxPower(17);

           // put in standby mode
           idle();
           return 1;
          }

    void end()
          {
           // put in sleep mode
           sleep();

           // stop SPI
           _spi->end();
          }

    int beginPacket(int implicitHeader = false)
          {
           if (isTransmitting()) 
               return 0;

           // put in standby mode
           idle();

           if (implicitHeader)
               implicitHeaderMode();
           else 
               explicitHeaderMode();

           // reset FIFO address and paload length
           writeRegister(REG_FIFO_ADDR_PTR, 0);
           writeRegister(REG_PAYLOAD_LENGTH, 0);
           return 1;
          }

    int endPacket(bool async = false)
          {
           if ((async) && (_onTxDone))
               writeRegister(REG_DIO_MAPPING_1, 0x40); // DIO0 => TXDONE

           // put in TX mode
           writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

           if (!async) 
              {
               // wait for TX done
               while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0)
                      yield();
               // clear IRQ's
               writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
              }
           return 1;
          }

    int parsePacket(int size)
          {
           int packetLength = 0;
           int irqFlags = readRegister(REG_IRQ_FLAGS);
           if (size > 0) 
              {
               implicitHeaderMode();
               writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
              } 
           else
               explicitHeaderMode();

           // clear IRQ's
           writeRegister(REG_IRQ_FLAGS, irqFlags);
           if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) 
              {
               // received a packet
               _packetIndex = 0;

               // read packet length
               if (_implicitHeaderMode)
                   packetLength = readRegister(REG_PAYLOAD_LENGTH);
               else 
                   packetLength = readRegister(REG_RX_NB_BYTES);
    
               // set FIFO address to current RX address
               writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

               // put in standby mode
               idle();
              }
           else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) 
              {
               // not currently in RX mode
               // reset FIFO address
               writeRegister(REG_FIFO_ADDR_PTR, 0);
               // put in single RX mode
               writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
              }
           return packetLength;
          }
    
    int packetRssi()
          {
           return (readRegister(REG_PKT_RSSI_VALUE) - (_frequency < 868E6 ? 164 : 157));
          }

    float packetSnr()
          {
           return ((int8_t)readRegister(REG_PKT_SNR_VALUE)) * 0.25;
          }

    long packetFrequencyError()
          {
           int32_t freqError = 0;
           freqError = static_cast<int32_t>(readRegister(REG_FREQ_ERROR_MSB) & B111);
           freqError <<= 8L;
           freqError += static_cast<int32_t>(readRegister(REG_FREQ_ERROR_MID));
           freqError <<= 8L;
           freqError += static_cast<int32_t>(readRegister(REG_FREQ_ERROR_LSB));
           if (readRegister(REG_FREQ_ERROR_MSB) & B1000) 
              { // Sign bit is on
               freqError -= 524288; // B1000'0000'0000'0000'0000
              }
           const float fXtal = 32E6; // FXOSC: crystal oscillator (XTAL) frequency (2.5. Chip Specification, p. 14)
           const float fError = ((static_cast<float>(freqError) * (1L << 24)) / fXtal) * (getSignalBandwidth() / 500000.0f); // p. 37
           return static_cast<long>(fError);
          }

    // from Print
    //virtual size_t write(uint8_t byte);
    virtual size_t write(uint8_t byte)
          {
           return write(&byte, sizeof(byte));
          }
    
    //virtual size_t write(const uint8_t *buffer, size_t size);
    virtual size_t write(const uint8_t *buffer, size_t size)
          {
           int currentLength = readRegister(REG_PAYLOAD_LENGTH);
           // check size
           if ((currentLength + size) > MAX_PKT_LENGTH) 
               size = MAX_PKT_LENGTH - currentLength;
           // write data
           for (size_t i = 0; i < size; i++)
                writeRegister(REG_FIFO, buffer[i]);
           // update length
           writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);
           return size;
          }
    
    // from Stream
    //virtual int available();
    virtual int available()
        {
         return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
        }
    
    //virtual int read();
    virtual int read()
          {
           if (!available()) 
               return -1;
           _packetIndex++;
           return readRegister(REG_FIFO);
           }

    //virtual int peek();
    virtual int peek()
          {
           if (!available())
               return -1;
           // store current FIFO address
           int currentAddress = readRegister(REG_FIFO_ADDR_PTR);
           // read
           uint8_t b = readRegister(REG_FIFO);
           // restore FIFO address
           writeRegister(REG_FIFO_ADDR_PTR, currentAddress);
           return b;
           }

    virtual void flush() {};
    
    void onReceive(void(*callback)(int))
          {
           _onReceive = callback;
           if (callback) 
              {
               pinMode(_dio0, INPUT);
               #ifdef SPI_HAS_NOTUSINGINTERRUPT
                  SPI.usingInterrupt(digitalPinToInterrupt(_dio0));
               #endif
               attachInterrupt(digitalPinToInterrupt(_dio0), onDio0Rise, RISING);
              } 
           else 
              {
               detachInterrupt(digitalPinToInterrupt(_dio0));
               #ifdef SPI_HAS_NOTUSINGINTERRUPT
                  SPI.notUsingInterrupt(digitalPinToInterrupt(_dio0));
               #endif
              }
          }

    void onTxDone(void(*callback)())
          {
           _onTxDone = callback;
           if (callback) 
              {
               pinMode(_dio0, INPUT);
               #ifdef SPI_HAS_NOTUSINGINTERRUPT
                  SPI.usingInterrupt(digitalPinToInterrupt(_dio0));
               #endif
               attachInterrupt(digitalPinToInterrupt(_dio0), onDio0Rise, RISING);
              } 
           else 
              {
               detachInterrupt(digitalPinToInterrupt(_dio0));
               #ifdef SPI_HAS_NOTUSINGINTERRUPT
                  SPI.notUsingInterrupt(digitalPinToInterrupt(_dio0));
               #endif
              }
          }

    void onRxTimeOut(void(*callback)())
          {
           _onRxTimeOut = callback;
           if (callback) 
              {
               pinMode(_dio1, INPUT);
               #ifdef SPI_HAS_NOTUSINGINTERRUPT
                  SPI.usingInterrupt(digitalPinToInterrupt(_dio1));
               #endif
               attachInterrupt(digitalPinToInterrupt(_dio1), onDio1Rise, RISING);
              } 
           else 
              {
               detachInterrupt(digitalPinToInterrupt(_dio1));
               #ifdef SPI_HAS_NOTUSINGINTERRUPT
                  SPI.notUsingInterrupt(digitalPinToInterrupt(_dio1));
               #endif
              }
          }

    void receive(int size = 0)
          {
           writeRegister(REG_DIO_MAPPING_1, 0x00); // DIO0 => RXDONE
           if (size > 0) 
              {
               implicitHeaderMode();
               writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
              } 
           else 
              {
               explicitHeaderMode();
              }
           writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
          }

    void receive_single(int size = 0)
          {
           writeRegister(REG_DIO_MAPPING_1, 0x00); // DIO0 => RXDONE
           if (size > 0) 
              {
               implicitHeaderMode();
               writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
              } 
           else 
              {
               explicitHeaderMode();
              }
           writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
          }

    void idle()
          {
           writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
          }

    void sleep()
          {
           writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
          }

    void setTxPower(int level, int outputPin = PA_OUTPUT_PA_BOOST_PIN)
          {
           if (PA_OUTPUT_RFO_PIN == outputPin) 
              {
               // RFO
               if (level < 0)
                   level = 0;
               else if (level > 14)
                   level = 14;
               writeRegister(REG_PA_CONFIG, 0x70 | level);
              } 
           else 
              {
               // PA BOOST
               if (level > 17)
                  {
                   if (level > 20)
                       level = 20;
                   // subtract 3 from level, so 18 - 20 maps to 15 - 17
                   level -= 3;
                   // High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
                   writeRegister(REG_PA_DAC, 0x87);
                   setOCP(140);
                  } 
               else 
                  {
                   if (level < 2) 
                       level = 2; 
                   //Default value PA_HF/LF or +17dBm
                   writeRegister(REG_PA_DAC, 0x84);
                   setOCP(100);
                  }
               writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
              }
          }

    void setFrequency(long frequency)
          {
           _frequency = frequency;
           uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
           writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
           writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
           writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
          }

    void setSpreadingFactor(int sf)
          {
           if (sf < 6)
               sf = 6;
           else if (sf > 12)
               sf = 12;
           if (sf == 6) 
              {
               writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
               writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
              } 
           else 
              {
               writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
               writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
              }
           writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
           setLdoFlag();
          }

    void setSignalBandwidth(long sbw)
          {
           int bw;
           if (sbw <= 7.8E3)
               bw = 0;
           else if (sbw <= 10.4E3)
               bw = 1;
           else if (sbw <= 15.6E3)
               bw = 2;
           else if (sbw <= 20.8E3)
               bw = 3;
           else if (sbw <= 31.25E3)
               bw = 4;
           else if (sbw <= 41.7E3)
               bw = 5;
           else if (sbw <= 62.5E3)
               bw = 6;
           else if (sbw <= 125E3)
               bw = 7;
           else if (sbw <= 250E3)
               bw = 8;
           else /*if (sbw <= 250E3)*/
               bw = 9;
           writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
           setLdoFlag();
          }

    void setCodingRate4(int denominator)
          {
           if (denominator < 5)
               denominator = 5;
           else if (denominator > 8)
               denominator = 8;
           int cr = denominator - 4;
           writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
          }

    void setPreambleLength(long length)
          {
           writeRegister(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
           writeRegister(REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
          }

    void setSyncWord(int sw)
          {
           writeRegister(REG_SYNC_WORD, sw);
          }

    byte getSyncWord()
          {
           return(readRegister(REG_SYNC_WORD)); 
          }

    void enableCrc()
          {
           writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) | 0x04);
          }

    void disableCrc()
          {
           writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) & 0xfb);
          }

    void enableInvertIQ()
          {
           writeRegister(REG_INVERTIQ,  0x66);
           writeRegister(REG_INVERTIQ2, 0x19);
          }

    void disableInvertIQ()
          {
           writeRegister(REG_INVERTIQ,  0x27);
           writeRegister(REG_INVERTIQ2, 0x1d);
          }

    void setRxSymbTimeout(byte symbols)
          {
           writeRegister(REG_SYMB_TIMEOUT_LSB,  symbols);
          }

    void setOCP(uint8_t mA)
          {
           uint8_t ocpTrim = 27;
           if (mA <= 120)
               ocpTrim = (mA - 45) / 5;
           else if (mA <=240)
               ocpTrim = (mA + 30) / 10;
           writeRegister(REG_OCP, 0x20 | (0x1F & ocpTrim));
          }

    // deprecated
    void crc() { enableCrc(); }
    void noCrc() { disableCrc(); }

    byte random()
          {
           return readRegister(REG_RSSI_WIDEBAND);
          }

    void setPins(int ss, int reset, int dio0, int dio1)
          {
           _ss = ss;
           _reset = reset;
           _dio0 = dio0;
           _dio1 = dio1;
          }

    void setSPI(SPIClass& spi)
          {
           _spi = &spi;
          }

    void setSPIFrequency(uint32_t frequency)
          {
           _spiSettings = SPISettings(frequency, MSBFIRST, SPI_MODE0);
          }

    void dumpRegisters(Stream& out)
          {
           for (int i = 0; i < 128; i++) 
               {
                out.print("0x");
                out.print(i, HEX);
                out.print(": 0x");
                out.println(readRegister(i), HEX);
               }
          }
  //----------------------------------------------------------------- PUBLIC -----------------------------------------------------------------------
 }; //Τέλος ορισμού της κλάσης

LoRaClass LoRa;

 ISR_PREFIX void LoRaClass::onDio0Rise()
              {
               LoRa.handleDio0Rise();
              }

 ISR_PREFIX void LoRaClass::onDio1Rise()
              {
               LoRa.handleDio1Rise();
              }
