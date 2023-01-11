/*
 *  temperature sensor on analog 8 to test the LoRa gateway
 *
 *  Copyright (C) 2016 Congduc Pham, University of Pau, France
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************
 * last update: Nov. 26th by C. Pham
 */
#include "lora.h"

#define ARDUINO_AVR_PRO
#define LED_PIN 7

#define ETSI_EUROPE_REGULATION
#define BAND868

#define MAX_DBM 14

const uint32_t DEFAULT_CHANNEL=CH_10_868;

#define PABOOST

// #define WITH_EEPROM
// #define WITH_APPKEY
#define FLOAT_TEMP
#define NEW_DATA_FIELD
#define LOW_POWER
#define LOW_POWER_HIBERNATE
#define WITH_ACK

// CHANGE HERE THE LORA MODE, NODE ADDRESS
#define LORAMODE  1
#define node_addr 8

// CHANGE HERE THE THINGSPEAK FIELD BETWEEN 1 AND 4
#define field_index 1

// CHANGE HERE THE TIME IN MINUTES BETWEEN 2 READING & TRANSMISSION
unsigned int idlePeriodInMin = 10;
///////////////////////////////////////////////////////////////////

#ifdef WITH_APPKEY
    // CHANGE HERE THE APPKEY, BUT IF GW CHECKS FOR APPKEY, MUST BE
    // IN THE APPKEY LIST MAINTAINED BY GW.
    uint8_t my_appKey[4]={5, 6, 7, 8};
#endif

// we wrapped Serial.println to support the Arduino Zero or M0
#define PRINTLN                   Serial.println("")
#define PRINT_CSTSTR(fmt,param)   Serial.print(F(param))
#define PRINT_STR(fmt,param)      Serial.print(param)
#define PRINT_VALUE(fmt,param)    Serial.print(param)
#define FLUSHOUTPUT               Serial.flush();

#ifdef WITH_EEPROM
    #include <EEPROM.h>
#endif

#define DEFAULT_DEST_ADDR 1

#ifdef WITH_ACK
    #define NB_RETRIES 3
#endif

#ifdef LOW_POWER
    #define LOW_POWER_PERIOD 8
    // you need the LowPower library from RocketScream
    // https://github.com/rocketscream/Low-Power
    #include "LowPower.h"

    unsigned int nCycle = idlePeriodInMin*60/LOW_POWER_PERIOD;
#endif

double temp;
unsigned long nextTransmissionTime=0L;
char float_str[220] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin ut tellus faucibus, scelerisque lorem in, maximus lectus. Suspendisse rutrum enim mi, ac condimentum dolor tincidunt ac. Vestibulum amet.";
uint8_t message[240];
int loraMode=LORAMODE;

#ifdef WITH_EEPROM
    struct sx1272config {

      uint8_t flag1;
      uint8_t flag2;
      uint8_t seq;
      // can add other fields such as LoRa mode,...
    };

    sx1272config my_sx1272config;
#endif

void setup_lora()
{
  pinMode(LED_PIN, OUTPUT);
  int e;

  // Power ON the module
  sx1272.ON();

#ifdef WITH_EEPROM
  // get config from EEPROM
  EEPROM.get(0, my_sx1272config);

  // found a valid config?
  if (my_sx1272config.flag1==0x12 && my_sx1272config.flag2==0x34) {
    PRINT_CSTSTR("%s","Get back previous sx1272 config\n");

    // set sequence number for SX1272 library
    sx1272._packetNumber=my_sx1272config.seq;
    PRINT_CSTSTR("%s","Using packet sequence number of ");
    PRINT_VALUE("%d", sx1272._packetNumber);
    PRINTLN;
  }
  else {
    // otherwise, write config and start over
    my_sx1272config.flag1=0x12;
    my_sx1272config.flag2=0x34;
    my_sx1272config.seq=sx1272._packetNumber;
  }
#endif

  // Set transmission mode and print the result
  e = sx1272.setMode(loraMode);
  PRINT_CSTSTR("%s","Setting Mode: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // enable carrier sense
  sx1272._enableCarrierSense=true;
#ifdef LOW_POWER
  // TODO: with low power, when setting the radio module in sleep mode
  // there seem to be some issue with RSSI reading
  sx1272._RSSIonSend=false;
#endif

  // Select frequency channel
  e = sx1272.setChannel(DEFAULT_CHANNEL);
  PRINT_CSTSTR("%s","Setting Channel: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Select amplifier line; PABOOST or RFO
#ifdef PABOOST
    sx1272._needPABOOST=true;
#endif

  e = sx1272.setPowerDBM((uint8_t)MAX_DBM);
  PRINT_CSTSTR("%s","Setting Power: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Set the node address and print the result
  e = sx1272.setNodeAddress(node_addr);
  PRINT_CSTSTR("%s","Setting node addr: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Print a success message
  PRINT_CSTSTR("%s","SX1272 successfully configured\n");

  delay(500);
}

#if not defined _VARIANT_ARDUINO_DUE_X_ && defined FLOAT_TEMP

char *ftoa(char *a, double f, int precision)
{
 long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};

 char *ret = a;
 long heiltal = (long)f;
 itoa(heiltal, a, 10);
 while (*a != '\0') a++;
 *a++ = '.';
 long desimal = abs((long)((f - heiltal) * p[precision]));
 itoa(desimal, a, 10);
 return ret;
}
#endif

void send_message(uint8_t *text)
{
  long startSend;
  long endSend;
  uint8_t app_key_offset=0;
  int e;

#ifdef WITH_APPKEY
      app_key_offset = sizeof(my_appKey);
      // set the app key in the payload
      memcpy(message,my_appKey,app_key_offset);
#endif

    uint8_t r_size;

    r_size=sprintf((char*)message+app_key_offset,"\\!#%s", text);


      PRINT_CSTSTR("%s","Sending ");
      PRINT_STR("%s",(char*)(message+app_key_offset));
      PRINTLN;

      PRINT_CSTSTR("%s","Real payload size is ");
      PRINT_VALUE("%d", r_size);
      PRINTLN;

      int pl=r_size+app_key_offset;

      sx1272.CarrierSense();

      startSend=millis();

#ifdef WITH_APPKEY
      // indicate that we have an appkey
      sx1272.setPacketType(PKT_TYPE_DATA | PKT_FLAG_DATA_WAPPKEY);
#else
      // just a simple data packet
      sx1272.setPacketType(PKT_TYPE_DATA);
#endif

      // Send message to the gateway and print the result
      // with the app key if this feature is enabled
#ifdef WITH_ACK
      int n_retry=NB_RETRIES;

      do {
        e = sx1272.sendPacketTimeoutACK(DEFAULT_DEST_ADDR, message, pl);

        if (e==3)
          PRINT_CSTSTR("%s","No ACK");

        n_retry--;

        if (n_retry)
          PRINT_CSTSTR("%s","Retry");
        else
          PRINT_CSTSTR("%s","Abort");

      } while (e && n_retry);
#else
      e = sx1272.sendPacketTimeout(DEFAULT_DEST_ADDR, message, pl);
#endif
      endSend=millis();

#ifdef WITH_EEPROM
      // save packet number for next packet in case of reboot
      my_sx1272config.seq=sx1272._packetNumber;
      EEPROM.put(0, my_sx1272config);
#endif

      PRINT_CSTSTR("%s","LoRa pkt size ");
      PRINT_VALUE("%d", pl);
      PRINTLN;

      PRINT_CSTSTR("%s","LoRa pkt seq ");
      PRINT_VALUE("%d", sx1272.packet_sent.packnum);
      PRINTLN;

      PRINT_CSTSTR("%s","LoRa Sent in ");
      PRINT_VALUE("%ld", endSend-startSend);
      PRINTLN;

      PRINT_CSTSTR("%s","LoRa Sent w/CAD in ");
      PRINT_VALUE("%ld", endSend-sx1272._startDoCad);
      PRINTLN;

      PRINT_CSTSTR("%s","Packet sent, state ");
      PRINT_VALUE("%d", e);
      PRINTLN;

      if (!e) {
          digitalWrite(LED_PIN, HIGH);
          delay(5000);
          digitalWrite(LED_PIN, LOW);
      } else {
          for (int x = 0; x < 10; x++) {
              digitalWrite(LED_PIN, HIGH);
              delay(250);
              digitalWrite(LED_PIN, LOW);
              delay(250);
          }
      }

      PRINT_CSTSTR("%s","Remaining ToA is ");
      PRINT_VALUE("%d", sx1272.getRemainingToA());
      PRINTLN;

#ifdef LOW_POWER
      PRINT_CSTSTR("%s","Switch to power saving mode\n");

      e = sx1272.setSleepMode();

      if (!e)
        PRINT_CSTSTR("%s","Successfully switch LoRa module in sleep mode\n");
      else
        PRINT_CSTSTR("%s","Could not switch LoRa module in sleep mode\n");

      FLUSHOUTPUT
      delay(50);
#endif
}
