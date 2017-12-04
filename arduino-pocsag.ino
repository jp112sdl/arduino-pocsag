/**
    Decodes POCSAG data from the FSK demodulator output on the RF board of Motorola pagers.
    Tested successfully with up to 1200 baud POCSAG

    Copyright (C) 2013  Tom de Waha

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**/
#include <TimerOne.h>
#include <EEPROM.h>
#include <Wire.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include "help_message.h"

#define RTC_I2C_ADDRESS 0x68
#define receiverPin                   	19 // Input from Receiver Board
#define pmbledPin         	             8 // LED if Preamble is detected
#define syncledPin                       9 // LED if Sync Word is detected
#define cwerrledPin                     10 // LED if there were too many codewords with errors
#define fsaledPin                       11 // LED for field strength alarm
#define bitPeriod                      833
#define halfBitPeriod                  416
#define halfBitPeriodTolerance          30
#define prmbWord                1431655765
#define syncWord                2094142936
#define idleWord                2055848343

#define STATE_WAIT_FOR_PRMB 	  0
#define STATE_WAIT_FOR_SYNC 	  1
#define STATE_PROCESS_BATCH 	  2
#define STATE_PROCESS_MESSAGE 	3

#define MSGLENGTH 	                244
#define BITCOUNTERLENGTH	          440
#define MAXNUMBATCHES		             32

static const char *functions[4] = {"A", "B", "C", "D"};
enum {OFF, ON};
enum {DL_OFF, DL_LOW, DL_MAX};
volatile unsigned long buffer = 0;
volatile int bitcounter = 0;
int state = 0;
int wordcounter = 0;
int framecounter = 0;
int batchcounter = 0;
unsigned long wordbuffer[(MAXNUMBATCHES * 16) + 1];
byte cw[32];
unsigned int bch[1025], ecs[25];
unsigned long last_pmb_millis = 0;
bool field_strength_alarm = false;
unsigned long cwerrled_on = 0;
byte decode_errorcount = 0;

struct userconfig_t {
  byte DebugLevel = 0;
  bool enable_paritycheck = false;
  byte ecc_mode = 0;
  bool enable_led = false;
  bool enable_rtc = false;
  bool enable_umlautreplace = true;
  bool enable_emptymsg = true;
  byte invert_signal = RISING;
  byte fsa_timeout_minutes = 10;
  byte max_allowd_cw_errors = 8;
  uint32_t fromRIC = 0;
  uint32_t toRIC = 0;
} UserConfig;

//RTC Variablen
int jahr, monat, tag, stunde, minute, sekunde, wochentag;
int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char serialbuffer[30];
byte serialbuffer_counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("START POCSAG DECODER");

  init_gpio();

  init_eeprom();

  if (UserConfig.enable_rtc) Wire.begin();

  print_config();
  start_flank();
  Timer1.initialize(bitPeriod);
  init_led();
}

void loop() {
  switch (state) {
    case STATE_WAIT_FOR_PRMB:
      if (buffer == prmbWord) {
        state = STATE_WAIT_FOR_SYNC;
        set_pmbled(ON);
        if (UserConfig.fsa_timeout_minutes > 0) {
          last_pmb_millis = millis();
          if (UserConfig.enable_led && field_strength_alarm) set_fsaled(OFF);
          field_strength_alarm = false;
        }
      }
      break;

    case STATE_WAIT_FOR_SYNC:
      if (buffer == syncWord) {
        set_syncled(ON);
        bitcounter = 0;
        state = STATE_PROCESS_BATCH;
      } else {
        if (bitcounter > BITCOUNTERLENGTH) {
          bitcounter = 0;
          if (batchcounter > 0) {
            if (state != STATE_PROCESS_MESSAGE) {
              set_syncled(OFF);
              set_pmbled(OFF);
            }
            state = STATE_PROCESS_MESSAGE;
          } else {
            state = STATE_WAIT_FOR_PRMB;
            set_syncled(OFF);
            set_pmbled(OFF);
          }
          batchcounter = 0;
        }
      }
      break;

    case STATE_PROCESS_BATCH:
      if (bitcounter >= 32) {
        bitcounter = 0;
        wordbuffer[(batchcounter * 16) + (framecounter * 2) + wordcounter] = buffer;
        wordcounter++;
      }

      if (wordcounter >= 2) {
        wordcounter = 0;
        framecounter++;
      }

      if (framecounter >= 8) {
        framecounter = 0;
        batchcounter++;
        state = STATE_WAIT_FOR_SYNC;
      }

      if (batchcounter >= MAXNUMBATCHES) {
        batchcounter = 0;
        state = STATE_PROCESS_MESSAGE;
      }
      break;

    case STATE_PROCESS_MESSAGE:
      stop_flank();
      stop_timer();

      decode_wordbuffer();

      memset(wordbuffer, 0, sizeof(wordbuffer));
      state = STATE_WAIT_FOR_PRMB;
      set_syncled(OFF);
      set_pmbled(OFF);
      start_flank();
      break;
  }

  if (UserConfig.fsa_timeout_minutes > 0) {
    if (last_pmb_millis > millis())
      last_pmb_millis = millis();
    if (!field_strength_alarm && (last_pmb_millis == 0 || millis() - last_pmb_millis > UserConfig.fsa_timeout_minutes * 60000)) {
      Serial.println("\r\n=== [" + strRTCDateTime() + "] +++ Field Strength Alarm! +++");
      field_strength_alarm = true;
      set_fsaled(ON);
    }
  }

  if (millis() - cwerrled_on > 2000)
    set_cwerrled(OFF);


  if (Serial.available()) {
    delay(100);
    while (Serial.available()) {
      char inChr = Serial.read();
      serialbuffer[serialbuffer_counter] = inChr;
      Serial.print(serialbuffer[serialbuffer_counter]);
      if (inChr == '\r' || inChr == '\n') {
        process_serial_input();
        memset(serialbuffer, 0, sizeof(serialbuffer));
        serialbuffer_counter = 0;
      } else if (serialbuffer_counter < sizeof(serialbuffer) - 1 && inChr != '\r') serialbuffer_counter++;
    }
  }
}

void decode_wordbuffer() {
  int address_counter = 0;
  unsigned long address[16];
  memset(address, 0, sizeof(address));
  byte function[16];
  memset(function, 0, sizeof(function));
  char message[MSGLENGTH];
  memset(message, 0, sizeof(message));
  byte character = 0;
  int bcounter = 0;
  int ccounter = 0;
  int used_cw_counter = 0;
  boolean eot = false;
  unsigned long start_millis = millis();
  byte address_idx_wo_msg[16];
  byte address_idx_wo_msg_counter = 0;

  for (int i = 0; i < ((MAXNUMBATCHES * 16) + 1); i++) {
    if (wordbuffer[i] == 0) continue;
    if (UserConfig.DebugLevel == DL_MAX )
      Serial.print("\r\ncw[" + String(i) + "] = " + String(wordbuffer[i]) + ";");

    used_cw_counter++;

    if (wordbuffer[i] == idleWord) continue;

    if (UserConfig.enable_paritycheck) {
      if (parity(wordbuffer[i]) == 1) {
        if (UserConfig.DebugLevel == DL_MAX) Serial.print("// PE");
        set_cwerrled(ON);
        continue;
      }
    }

    if (UserConfig.ecc_mode > 0) {
      unsigned long preEccWb = wordbuffer[i];
      for (int cw_bcounter = 0; cw_bcounter < 32; cw_bcounter++)
        cw[cw_bcounter] = bitRead(wordbuffer[i], 31 - cw_bcounter);

      int ecdcount = ecd();

      if (ecdcount == 3) decode_errorcount++;

      if (decode_errorcount > UserConfig.max_allowd_cw_errors) {
        set_cwerrled(ON);
        if (UserConfig.DebugLevel == DL_MAX)
          Serial.print("\r\ndecode_wordbuffer process cancelled! too much errors. errorcount > " + String(UserConfig.max_allowd_cw_errors));
        break;
      }

      if (ecdcount < UserConfig.ecc_mode + 1)
        for (int cw_bcounter = 0; cw_bcounter < 32; cw_bcounter++)
          bitWrite(wordbuffer[i], 31 - cw_bcounter, (int)cw[cw_bcounter]);

      if (UserConfig.DebugLevel == DL_MAX) {
        Serial.print(" // (" + String(ecdcount) + ") ");
        if (preEccWb != wordbuffer[i])
          Serial.print("*");
      }
    }

    if (bitRead(wordbuffer[i], 31) == 0) {
      if  ((i > 0 || address_counter == 0) && (parity(wordbuffer[i]) != 1)) {
        address[address_counter] = extract_address(i);
        if (UserConfig.DebugLevel == DL_MAX) Serial.print(" //address " + String(address[address_counter]) + " found, address_counter = " + String(address_counter));
        function[address_counter] = extract_function(i);
        if (address_counter > 0) {
          if (ccounter > 0) {
            print_message(address[address_counter - 1], function[address_counter - 1], message);
            for (int idx_wo_msg = 0; idx_wo_msg < address_idx_wo_msg_counter; idx_wo_msg++) print_message(address[address_idx_wo_msg[idx_wo_msg]], function[address_idx_wo_msg[idx_wo_msg]], message);
            address_idx_wo_msg_counter = 0;
          } else {
            address_idx_wo_msg[address_idx_wo_msg_counter] = address_counter - 1;
            address_idx_wo_msg_counter++;
          }
        }
        eot = false;
        memset(message, 0, sizeof(message));
        ccounter = 0;
        bcounter = 0;
        address_counter++;
      }
    } else {

      if (address[address_counter - 1] != 0 && ccounter < MSGLENGTH) {
        for (int c = 30; c > 10; c--) {
          bitWrite(character, bcounter, bitRead(wordbuffer[i], c));
          bcounter++;
          if (bcounter >= 7) {
            if (character == 4) {
              message[ccounter] = 127;
              eot = true;
            }
            bcounter = 0;
            if (eot == false) {
              message[ccounter] = character;
              ccounter++;
            }
          }
        }
      }
    }
  }

  if (address_counter > 0) {
    print_message(address[address_counter - 1], function[address_counter - 1], message);
    if (UserConfig.DebugLevel == DL_MAX)  Serial.print("\r\naddress_counter = " + String(address_counter));
  }
  if (UserConfig.DebugLevel > DL_OFF) Serial.print("\r\n=== [" + strRTCDateTime() + "] CW(" + String(used_cw_counter) + ") E(" + String(decode_errorcount) + ") " + String(millis() - start_millis) + "ms ===");
  decode_errorcount = 0;
}
