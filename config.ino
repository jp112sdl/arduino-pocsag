void print_config() {
  String strpcheck =  ((UserConfig.enable_paritycheck == true) ?        F("Parity Check           ON")                                               :         F("Parity Check           OFF"));
  String struml    =  ((UserConfig.enable_umlautreplace == true) ?      F("Umlaut-Replace         ON")                                               :         F("Umlaut-Replace         OFF"));
  String strrtc    =  ((UserConfig.enable_rtc         == true) ? String(F("Real Time Clock        ON (")) + strRTCDateTime() + F(")")                 : String(F("Real Time Clock        OFF")));
  String strecc =                                                       F("ECC-Mode               OFF");
  if (UserConfig.ecc_mode == 1) strecc =                                F("ECC-Mode               1 bit");
  if (UserConfig.ecc_mode == 2) strecc =                                F("ECC-Mode               2 bit");
  if (UserConfig.ecc_mode == 3) strecc =                                F("ECC-Mode               >2 bit");
  String strmax_allowd_cw_errors =                               String(F("Max. # of CW w/ Errors ")) + String(UserConfig.max_allowd_cw_errors);
  String strled =  ((UserConfig.enable_led == true) ?                   F("LED                    ON")                                               :        F("LED                    OFF"));
  String strfsa =  ((UserConfig.fsa_timeout_minutes > 0) ?       String(F("Field Strength Alarm   ON (")) + String(UserConfig.fsa_timeout_minutes) + " min.)"   : String(F("Field Strength Alarm   OFF")));
  String strdebug =                                                     F("Debug Level            OFF (0)");
  if (UserConfig.debugLevel == 1) strdebug =                            F("Debug Level            CW 0+1 ");
  if (UserConfig.debugLevel == 2) strdebug =                            F("Debug Level            ALL (2)");
  String strinvert =  ((UserConfig.invert_signal == FALLING) ?          F("Input Level            NORMAL")                                           :        F("Input Level            INV."));
  String strricfilter = ((UserConfig.fromRIC != 0 && UserConfig.toRIC != 0) ? String(F("RIC-Filter             ")) + String(UserConfig.fromRIC) + " - " + String(UserConfig.toRIC) : String(F("RIC-Filter             OFF")));

  Serial.println(String(F("******** current config ********\r\n")) + strpcheck + F("\r\n") + strdebug + F("\r\n") + strecc + F("\r\n") + strmax_allowd_cw_errors + F("\r\n") + strled + F("\r\n") + strinvert + F("\r\n") + strfsa + F("\r\n") + strrtc + F("\r\n") + struml + F("\r\n") + strricfilter);
}

void process_serial_input() {
  if (serialbuffer_counter > 0) {
    Serial.println();
    if (strstr(serialbuffer, "time")) {
      if (serialbuffer_counter > 23) {
        tag = getIntFromString (serialbuffer, 1);
        monat = getIntFromString (serialbuffer, 2);
        jahr = getIntFromString (serialbuffer, 3);
        stunde = getIntFromString (serialbuffer, 4);
        minute = getIntFromString (serialbuffer, 5);
        sekunde = getIntFromString (serialbuffer, 6);

        if (!checkDateTime(jahr, monat, tag, stunde, minute, sekunde)) {
          Serial.println(serialbuffer);
          Serial.println(F("Example: time 08.11.2017 10:54:00"));
          return;
        }
        rtcWriteTime(jahr, monat, tag, stunde, minute, sekunde);
        Serial.println(F("Time and Date set."));
      }
      Serial.println(strRTCDateTime());
      return;
    }
    if (strstr(serialbuffer, "h") || strstr(serialbuffer, "?"))
      Serial.println(F("******** help config ********\r\nu0/1      = Umlaut-Replace dis-/enabled\r\nc0/1      = Real Time Clock dis-/enabled\r\np0/1      = Parity Check dis-/enabled\r\nd0/1/2    = Debug Level\r\ne0/1/2/3  = ECC (0) disabled, (1) 1 Bit, (2) 2 Bit, (3) >2 Bit\r\nl0/1      = LEDs dis-/enabled\r\ni0/1      = Input normal/inverted\r\nftnnn     = Field Strength Alarm (nnn minutes; 0 = off)\r\nmenn     = max. allowed codewords with errors\r\ntime      = time dd.mm.yyyy hh:mm:ss"));

    if (serialbuffer[0] == 'p')
      UserConfig.enable_paritycheck = getIntFromString(serialbuffer, 1);

    if (serialbuffer[0] == 'd')
      UserConfig.debugLevel = getIntFromString(serialbuffer, 1);

    if (serialbuffer[0] == 'e')
      UserConfig.ecc_mode = getIntFromString(serialbuffer, 1);

    if (serialbuffer[0] == 'l')
      UserConfig.enable_led = getIntFromString(serialbuffer, 1);

    if (serialbuffer[0] == 'u')
      UserConfig.enable_umlautreplace = getIntFromString(serialbuffer, 1);

    if (serialbuffer[0] == 'c')
      UserConfig.enable_rtc = getIntFromString(serialbuffer, 1);

    if (serialbuffer[0] == 'i') {
      if (getIntFromString(serialbuffer, 1) == 0)
        UserConfig.invert_signal = FALLING;
      else
        UserConfig.invert_signal = RISING;
    }

    if (strstr(serialbuffer, "rics")) {
      UserConfig.fromRIC = getIntFromString(serialbuffer, 1);
      UserConfig.toRIC   = getIntFromString(serialbuffer, 2);
    }

    if (strstr(serialbuffer, "ft"))
      UserConfig.fsa_timeout_minutes = getIntFromString(serialbuffer, 1);

    if (strstr(serialbuffer, "me"))
      UserConfig.max_allowd_cw_errors = getIntFromString(serialbuffer, 1);

    if (strstr(serialbuffer, "restart")) {
      softRestart();
    } else {
      eeprom_write_block((const void*)&UserConfig, (void*)0, sizeof(UserConfig));
      eeprom_read_userconfig();
      print_config();
    }
  }
}

void eeprom_read_userconfig() {
  eeprom_read_block((void*)&UserConfig, (void*)0, sizeof(UserConfig));
  if (UserConfig.ecc_mode > 0) setupecc();
  if (UserConfig.fromRIC == 4294950216) UserConfig.fromRIC = 0;
  if (UserConfig.toRIC == 4294950216) UserConfig.toRIC = 0;
  if (!UserConfig.enable_led) {
    disable_pmbled();
    disable_syncled();
    disable_fsaled();
  } else {
    if (field_strength_alarm) enable_fsaled();
  }

}
