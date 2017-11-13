const char help_message_text[] PROGMEM = R"=====(
###################################################################################################################
# p 0/1                     = dis-/enable Parity Check                                                            #
# d 0/1/2                   = Debug Level (0 = no debug | 1 = print cowdewords 1+2 | 2 = print all codewords)     #
# e 0/1/2/3                 = Error Correction (0) disabled | (1) 1 Bit- | (2) 2 Bit- | (3) >2 Bit-Errors         # 
# me <n>                    = max. <n> allowed codewords with errors                                              #
# i 0/1                     = Input Level normal/inverted (note: LX 4 Receiver uses inverted!)                    #
# ft <n>                    = Field Strength Alarm (<n> minutes | 0 = off)                                        #
# l 0/1                     = dis-/enable LEDs                                                                    #
# c 0/1                     = dis-/enable Real Time Clock                                                         #
# u 0/1                     = dis-/enable Umlaut-Replace                                                          #
# time dd.mm.yyyy hh:mm:ss  = set rtc time to dd.mm.yyyy hh:mm:ss ('time' w/o parameters prints current rtc time) #
# rics <a> <b>              = RIC Filter - output only from RIC <a> to RIC <b> (disable by setting a & b to 0)    #
###################################################################################################################


)=====";


