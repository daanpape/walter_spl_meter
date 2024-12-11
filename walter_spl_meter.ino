/**
 * @file walter_spl_meter.ino
 * @author Daan Pape <daan@dptechnics.com>
 * @date 9 Nov 2024
 * @copyright DPTechnics bv
 * @brief Cellular sound level sensor
 *
 * @section LICENSE
 *
 * Copyright (C) 2023, DPTechnics bv
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 * 
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 * 
 *   3. Neither the name of DPTechnics bv nor the names of its contributors may
 *      be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 * 
 *   4. This software, with or without modification, must only be used with a
 *      Walter board from DPTechnics bv.
 * 
 *   5. Any software provided in binary form under this license must not be
 *      reverse engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY DPTECHNICS BV “AS IS” AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL DPTECHNICS BV OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 * 
 * This sketch uploads sound level data to the walter demo platform every 60
 * seconds.
 */

#include <esp_mac.h>
#include <inttypes.h>
#include <WalterModem.h>
#include <DecibelMeter.hpp>
#include <Adafruit_SSD1306.h>

/**
 * @brief The address of the server to upload the data to. 
 */
#define SERV_ADDR "64.225.64.140"

/**
 * @brief The UDP port on which the server is listening.
 */
#define SERV_PORT 1999

/**
 * @brief The size in bytes of the SPL sensor packet.
 */
#define PACKET_SIZE 20

/**
 * @brief The serial interface to talk to the modem.
 */
#define ModemSerial Serial2

/**
 * @brief The radio access technology to use - LTEM or NBIOT.
 */
#define RADIO_TECHNOLOGY WALTER_MODEM_RAT_LTEM

/**
 * @brief I/O pin used for I2C SDA with the SPL meter.
 */
#define DB_METER_SDA 10

/**
 * @brief I/O pin used for I2C SCL with the SPL meter.
 */
#define DB_METER_SCL 9

/**
 * @brief The width of the OLED screen in pixels.
 */
#define SCREEN_WIDTH 128

/**
 * @brief The height of the OLED screen in pixels.
 */
#define SCREEN_HEIGHT 64

/**
 * @brief I/O pin used for I2C SDA with the OLED.
 */
#define SCREEN_SDA 16

/**
 * @brief I/O pin used for I2C SCL with the OLED.
 */
#define SCREEN_SCL 17

/**
 * @brief I/O pin used for the OLED reset line, -1 means not connected.
 */
#define SCREEN_RESET -1

/**
 * @brief The I2C address of the OLED screen.
 */
#define SCREEN_ADDRESS 0x3C

/**
 * @brief The modem instance.
 */
WalterModem modem;

/**
 * @brief The decibel meter instance.
 */
DecibelMeter dbmeter(DB_METER_SDA, DB_METER_SCL, 10000);

/**
 * @brief The I2C peripheral used for the screen.
 */
//TwoWire I2C1 = TwoWire(1);

/**
 * @brief The OLED screen instance.
 */
Adafruit_SSD1306 screen(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, SCREEN_RESET);

/**
 * @brief The buffer to transmit to the UDP server. The first 6 bytes will be
 * the MAC address of the Walter this code is running on.
 */
uint8_t dataBuf[PACKET_SIZE] = { 0 };

/**
 * @brief Connect to the LTE network.
 * 
 * This function will connect the modem to the LTE network. This function will
 * block until the modem is attached.
 * 
 * @return True on success, false on error.
 */
bool lteConnect()
{
  /* Set the operational state to full */
  if(!modem.setOpState(WALTER_MODEM_OPSTATE_FULL)) {
    Serial.print("Could not set operational state to FULL\r\n");
    return false;
  }

  /* Set the network operator selection to automatic */
  if(!modem.setNetworkSelectionMode(WALTER_MODEM_NETWORK_SEL_MODE_AUTOMATIC)) {
    Serial.print("Could not set the network selection mode to automatic\r\n");
    return false;
  }

  /* Wait for the network to become available */
  WalterModemNetworkRegState regState = modem.getNetworkRegState();
  while(!(regState == WALTER_MODEM_NETWORK_REG_REGISTERED_HOME ||
          regState == WALTER_MODEM_NETWORK_REG_REGISTERED_ROAMING))
  {
    delay(100);
    regState = modem.getNetworkRegState();
  }

  /* Stabilization time */
  Serial.print("Connected to the network\r\n");
  return true;
}

/**
 * @brief Connect to an UDP socket.
 * 
 * This function will set-up the modem and connect an UDP socket. The LTE 
 * connection must be active before this function can be called.
 * 
 * @param ip The IP address of the server to connect to.
 * @param port The port to connect to.
 * 
 * @return True on success, false on error.
 */
bool socketConnect(const char *ip, uint16_t port)
{
  /* Construct a socket */
  if(!modem.createSocket()) {
    Serial.print("Could not create a new socket\r\n");
    return false;
  }

  /* Configure the socket */
  if(!modem.configSocket()) {
    Serial.print("Could not configure the socket\r\n");
    return false;
  }

  /* Connect to the UDP test server */
  if(modem.connectSocket(ip, port, port)) {
    Serial.printf("Connected to UDP server %s:%d\r\n", ip, port);
  } else {
    Serial.print("Could not connect UDP socket\r\n");
    return false;
  }

  return true;
}

/**
 * @brief Display a status on the screen.
 * 
 * @param status A constant string representing the status of the SPL meter.
 * 
 * @return None.
 */
void displayStatus(const char *status)
{
  screen.setTextColor(SSD1306_WHITE);
  screen.clearDisplay();
  screen.setCursor(0, 0);
  screen.setTextSize(1);
  screen.print(status);
  screen.display();
}

/**
 * @brief Display SPL measurements on the screen.
 * 
 * @param db The current sound pressure level.
 * @param dbmin The minimum sound pressure level.
 * @param dbmax The maximum sound pressure level.
 * 
 * @return None.
 */
void displaySPL(uint8_t db, uint8_t dbmin, uint8_t dbmax)
{
  screen.setTextColor(SSD1306_WHITE);
  screen.clearDisplay();
  screen.setTextSize(2);
  screen.setCursor(0, 0);
  screen.printf("Avg: %ddB", db);
  screen.setCursor(0, 25);
  screen.printf("Min: %ddB", dbmin);
  screen.setCursor(0, 50);
  screen.printf("Max: %ddB", dbmax);
  screen.display();
}

/**
 * @brief Set up the system.
 * 
 * This function will set up the system and initialize the modem.
 * 
 * @return None.
 */
void setup()
{
  Serial.begin(115200);
  delay(5000);

  Serial.print("Sound Pressure Level sensor v0.0.2\r\n");

  /* We use IO8 as ground and IO18 as power for the decibel meter */
  pinMode(8, OUTPUT);
  pinMode(18, OUTPUT);
  digitalWrite(8, LOW);
  digitalWrite(18, HIGH);

  /* Switch on the 3V3 output and initialize the SSD1306 OLED screen */
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  delay(50);

  Wire1.begin(SCREEN_SDA, SCREEN_SCL, 400000);
  while(!screen.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    delay(1000);
  }

  displayStatus("Booting SPL v0.0.2");
  delay(1000);

  dbmeter.begin();
  Serial.printf("Decibel meter version: 0x%02X\n", dbmeter.getVersion());

  /* Configure 1000ms averaging and reset min/max */
  dbmeter.setAveragingInterval(1000);
  dbmeter.resetMinMax();

  /* Get the MAC address for board validation */
  esp_read_mac(dataBuf, ESP_MAC_WIFI_STA);
  Serial.printf("Walter's MAC is: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    dataBuf[0],
    dataBuf[1],
    dataBuf[2],
    dataBuf[3],
    dataBuf[4],
    dataBuf[5]);

  /* Modem initialization */
  displayStatus("Starting modem...");
  if(modem.begin(&ModemSerial)) {
    Serial.print("Modem initialization OK\r\n");
  } else {
    Serial.print("Modem initialization ERROR\r\n");
    delay(1000);
    ESP.restart();
    return;
  }

  WalterModemRsp rsp = {};
   if(modem.getRAT(&rsp)) {
     if(rsp.data.rat != RADIO_TECHNOLOGY) {
       modem.setRAT(RADIO_TECHNOLOGY);
       Serial.println("Switched modem radio technology");
     }
   } else {
     Serial.println("Could not retrieve radio access technology");
   }

  if(!modem.createPDPContext("")) {
    Serial.println("Could not create PDP context");
    delay(1000);
    ESP.restart();
    return;
  }

  /* Connect to the network */
  displayStatus("Connecting...");
  if(!lteConnect()) {
    Serial.print("Could not connect to the LTE network\r\n");
    delay(1000);
    ESP.restart();
    return;
  }
}

void loop()
{
  static WalterModemRsp rsp = {};

  /* Read the decibel meter's values */
  uint8_t db = dbmeter.readDecibel();
  uint8_t dbmin = dbmeter.readMinDecibel();
  uint8_t dbmax = dbmeter.readMaxDecibel();
  Serial.printf("dB = %03d [MIN: %03d, MAX: %03d]\n", db, dbmin, dbmax);

  displayStatus("Transmitting...");

  /* Reset the min/max registers */
  dbmeter.resetMinMax();

  /* Construct the decibel meter sensor packet */
  dataBuf[6] = db;
  dataBuf[7] = dbmin;
  dataBuf[8] = dbmax;


  if(!modem.getCellInformation(WALTER_MODEM_SQNMONI_REPORTS_SERVING_CELL, &rsp)) {
    Serial.println("Could not request cell information");
  } else {
    Serial.printf("Connected on band %u using operator %s (%u%02u)",
      rsp.data.cellInformation.band, rsp.data.cellInformation.netName,
      rsp.data.cellInformation.cc, rsp.data.cellInformation.nc);
    Serial.printf(" and cell ID %u.\r\n",
      rsp.data.cellInformation.cid);
    Serial.printf("Signal strength: RSRP: %.2f, RSRQ: %.2f.\r\n",
      rsp.data.cellInformation.rsrp, rsp.data.cellInformation.rsrq);

    /* Add monitor data to packet */
    dataBuf[9] = rsp.data.cellInformation.cc >> 8;
    dataBuf[10] = rsp.data.cellInformation.cc & 0xFF;
    dataBuf[11] = rsp.data.cellInformation.nc >> 8;
    dataBuf[12] = rsp.data.cellInformation.nc & 0xFF;
    dataBuf[13] = rsp.data.cellInformation.tac >> 8;
    dataBuf[14] = rsp.data.cellInformation.tac & 0xFF;
    dataBuf[15] = (rsp.data.cellInformation.cid >> 24) & 0xFF;
    dataBuf[16] = (rsp.data.cellInformation.cid >> 16) & 0xFF;
    dataBuf[17] = (rsp.data.cellInformation.cid >> 8) & 0xFF;
    dataBuf[18] = rsp.data.cellInformation.cid & 0xFF;
    dataBuf[19] = (uint8_t) (rsp.data.cellInformation.rsrp * -1);
  }

  if(!socketConnect(SERV_ADDR, SERV_PORT)) {
    Serial.print("Could not connect to UDP server socket\r\n");
    delay(1000);
    ESP.restart();
    return;
  }

  if(!modem.socketSend(dataBuf, PACKET_SIZE)) {
    Serial.print("Could not transmit data\r\n");
    delay(1000);
    ESP.restart();
    return;
  }

  delay(5000);

  /* Sometimes cell information is only available only after transmit */
  if(!modem.getCellInformation(WALTER_MODEM_SQNMONI_REPORTS_SERVING_CELL, &rsp)) {
      Serial.println("Could not request cell information");
    } else {
      Serial.printf("Connected on band %u using operator %s (%u%02u)",
        rsp.data.cellInformation.band, rsp.data.cellInformation.netName,
        rsp.data.cellInformation.cc, rsp.data.cellInformation.nc);
      Serial.printf(" and cell ID %u.\r\n",
        rsp.data.cellInformation.cid);
      Serial.printf("Signal strength: RSRP: %.2f, RSRQ: %.2f.\r\n",
        rsp.data.cellInformation.rsrp, rsp.data.cellInformation.rsrq);

      /* Add monitor data to packet */
      dataBuf[9] = rsp.data.cellInformation.cc >> 8;
      dataBuf[10] = rsp.data.cellInformation.cc & 0xFF;
      dataBuf[11] = rsp.data.cellInformation.nc >> 8;
      dataBuf[12] = rsp.data.cellInformation.nc & 0xFF;
      dataBuf[13] = rsp.data.cellInformation.tac >> 8;
      dataBuf[14] = rsp.data.cellInformation.tac & 0xFF;
      dataBuf[15] = (rsp.data.cellInformation.cid >> 24) & 0xFF;
      dataBuf[16] = (rsp.data.cellInformation.cid >> 16) & 0xFF;
      dataBuf[17] = (rsp.data.cellInformation.cid >> 8) & 0xFF;
      dataBuf[18] = rsp.data.cellInformation.cid & 0xFF;
      dataBuf[19] = (uint8_t) (rsp.data.cellInformation.rsrp * -1);
    }

  if(!modem.closeSocket()) {
    Serial.print("Could not close the socket\r\n");
    delay(1000);
    ESP.restart();
    return;
  }

  displaySPL(db, dbmin, dbmax);

  /* Light sleep for 60 seconds */
  // esp_sleep_enable_timer_wakeup(60 * 1000000);
  // esp_light_sleep_start();  
  delay(60000);
}
