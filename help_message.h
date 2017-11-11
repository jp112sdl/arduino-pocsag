const char help_message_text[] PROGMEM = R"=====(
#################################################################################################################
# u 0/1                     = dis-/enable Umlaut-Replace                                                        #
# c 0/1                     = dis-/enable Real Time Clock                                                       #
# p 0/1                     = dis-/enable Parity Check                                                          #
# l 0/1                     = dis-/enable LEDs                                                                  #
# d 0/1/2                   = Debug Level                                                                       #
# e 0/1/2/3                 = Error Correction (0) disabled, (1) 1 Bit-, (2) 2 Bit-, (3) >2 Bit-Errors          # 
# i 0/1                     = Input Level normal/inverted (LX4 Receiver uses inverted!)                         #
# ft <n>                    = Field Strength Alarm (<n> minutes; 0 = off)                                       #
# me <n>                    = max. <n> allowed codewords with errors                                            #
# time                      = prints current rtc time                                                           #
# time dd.mm.yyyy hh:mm:ss  = set rtc time to dd.mm.yyyy hh:mm:ss                                               #
# rics <a> <b>              = RIC Filter - output only from RIC <a> to RIC <b> (disable by setting a & b to 0)  #
#################################################################################################################


)=====";


