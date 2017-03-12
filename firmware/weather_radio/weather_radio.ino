/* Si4707 Breakout Example Code - I2C version
  by: Jim Lindblom
      SparkFun Electronics
  date: February 28, 2012
  
  license: Beerware. Please use, re-use, and modify this code without 
  restriction! We hope you find it useful. If you do, feel free to buy us
  a beer when we meet :).
  
  This example code shows off some of the functionality of the Si4707
  weather band receiver IC. Throw this code on your favorite Arduino
  and connect it like so:
  
    Si4707 Breakout -------------- Arduino
      3.3V  ----------------------  3.3V
      GND   ----------------------  GND
      SDIO  ----------------------  SDA (A4 on older 'duinos)
      SCLK  ----------------------  SCL (A5 on older 'duinos
      RST   ----------------------   4 (can be any digital pin)

  Any other pins on the Si4707 breakout can be left unconnected.
  The SEN pin is pulled high, which will affect the I2C address.
  The GPO1/2 pins are not used as outputs, and their default
  states set the Si4707 into I2C mode.
  
  For more information on the Si4707, check out the abundance of
  information provided by Silicon Labs:
  https://www.silabs.com/products/audiovideo/amfmreceivers/Pages/Si4707.aspx
  Specifically, check out the Programming Guide (AN332).
   
  When you start up the sketch, it will attempt to initialize the
  Si4707 and tune to the frequency defined by the tuneFrequency
  global variable. After tuning a menu will be printed to the 
  serial monitor, have fun interacting with the options presented!
*/
#include "si4707_definitions.h"
#include <Wire.h>

// In 2-wire mode, connecting the SEN pin to the Arduino is optional.
//  If it's not connected, make sure SEN_ADDRESS is defined as 1
//  assuming SEN is pulled high as on the breakout board.
#define SEN_ADDRESS 1
// In two-wire mode: if SEN is pulled high
#if SEN_ADDRESS
  #define SI4707_ADDRESS 0b1100011
#else  // Else if SEN is pulled low
  #define SI4707_ADDRESS 0b0010001
#endif
  
  
#define MAX_VOLUME 63
// Pin definitions:
// (If desired, these pins can be moved to other digital pins)
// SEN is optional, if not used, make sure SEN_ADDRESS is 1
const int senPin = 5;
const int rstPin = 4;
const int ledPin = 13;
const int intPin = 2;

// Not defined, because they must be connected to A4 and A5, 
//  are SDIO and SCLK.

// Put the WB frequency you'd like to tune to here:
// The value should be in kHz, so 162475 equates to 162.475 MHz
// The sketch will attempt to tune to this frequency when it starts.
// Find your frequency here: http://www.nws.noaa.gov/nwr/indexnw.htm
unsigned long tuneFrequency = 162400; // 162.400 MHz

// Initial volume level:
int userVol = 30;  // Maximum loudness (should be between 0 and 63)
boolean muted = false;

// for polling the SAME module
boolean recvingMsg = false;
byte blinkVal = 0; // blink the light if a SAME msg recvd (for debug only)

volatile boolean interrupt = false;
byte hdrCount = 0;
volatile byte timerVal = 0; // how many seconds since we received something
volatile boolean timerAlarm = false;
volatile boolean seekTuneActive = false;

// we set a timer for every second to check the same status
void createSameTimer()
{
    noInterrupts();
    // we want a 1Hz timer, so we need to use timer1
    TCCR1A = 0;
    TCCR1B = 0;
    
    TIMSK1 |= (1 << TOIE1);
    TCNT1 = 0x0BDC;

    blinkVal = 0;
    interrupts();
}

void clearTimer()
{
  cli();
  TCCR1B = 0;
  timerVal = 0;
  timerAlarm = false;
  sei();
}

void setTimer()
{
  cli();
  TCCR1B = (1 << CS12);
  timerVal = 0;
  timerAlarm = false;
  sei();
}

// ISR for the SAME status timer
ISR(TIMER1_OVF_vect)
{
    TCNT1 = 0x0BDC;
    timerAlarm = true;
}

// called if the timer expired, or a SAME msg succesfully rcvd
void timerExpiredMsgRcvd()
{
  clearTimer();
  hdrCount = 0;
  command_SAME_Status(2, 0, 1); // clear the SAME buffer

  setVolume(userVol); // set volume back to user's volume
  setMuteVolume(muted);
}

// The setup function initializes Serial, the Si4707, and tunes the Si4707
//  to the WB station defined at the top of this sketch (tuneFrequency).
//  To finish, it prints out the interaction menu over serial.
void setup()
{
    // Serial is used to interact with the menu, and to print debug info
    Serial.begin(9600);

    pinMode(ledPin, OUTPUT);
    pinMode(intPin, INPUT);
    digitalWrite(ledPin, 1);

    // First, initSi4707() must be called. The function returns the value
    //  of the Part Number reported by the Si4707, which can be checked
    //  to verify communication. It should always be 7.
    byte partNumber = initSi4707();
    if (partNumber == 7)
    {
        Serial.println("Successfully connected to Si4707");
    }
    else
    {
        Serial.print("Didn't connect to an Si4707...Error: ");
        Serial.println(partNumber);
        while(1) ;
    }

    // Enable interrupts
    setProperty(PROPERTY_GPO_IEN, 0x0007); // enable SAME, ASQ and STC interrupts
    // We tell SAME to interrupt on HDR_RDY, EOM_DET, and PRE_DET
    setProperty(PROPERTY_WB_SAME_INTERRUPT_SOURCE, 0x000B); // set SAME interrupts
    setProperty(PROPERTY_WB_ASQ_INTERRUPT_SOURCE, 0x0003); // set ASQ interrupt on ALERT on
    attachInterrupt(digitalPinToInterrupt(intPin), SameInterrupt, FALLING);

    // After initializing, we can tune to a WB frequency. Use the
    //  setWBFrequency() function to tune to a frequency. The frequency 
    //  parameter given to the function should be your chosen frequency in
    //  kHz. So to tune to 162.55 MHz, send 162550. The tuneFrequency
    //  variable is defined globablly near the top of this sketch.
    setWBFrequency(tuneFrequency);

    sameInit();
    sameTest();
    setVolume(userVol);
    digitalWrite(ledPin, 0);
    
    // After tuning, check out the serial monitor to interact with the menu.
    printMenu();
    
    createSameTimer();
    //setTimer();
}

void SameMsgRcvd()
{ 
  boolean rv = recvSameMsg();
  if (rv)
  {
      printSameMsg();
  }
  else
  {
      Serial.println(F("Error receiving SAME MSG!!"));
      printSameMsg(); // try to print it out for debugging  
  }

  processSameMsg();
}

void SameInterrupt()
{
  //if (digitalRead(intPin) == 0)
  {
    interrupt = true;
  }
}

void SameEvent()
{
  byte result = command_Get_Int_Status(NULL, NULL);
  
  boolean same_int = MASK_BYTE(result, SAMEINT_MASK);
  boolean asq_int = MASK_BYTE(result, ASQINT_MASK);
  boolean stc_int = MASK_BYTE(result, STCINT_MASK);
   
  if (same_int)
  {
    result = command_SAME_Status(1, 0, 0);
    if (MASK_BYTE(result, 0x8))
    {
      // EOM detected
      // clear SAME buffer, disable timer, HDR_COUNT = 0
      SameMsgRcvd();
      timerExpiredMsgRcvd();
    }
    else if (MASK_BYTE(result, 0x2))
    {
      // PRE detected
      setTimer();
    }
    else if (MASK_BYTE(result, 0x1))
    {
      // HDR RDY
      hdrCount++;
      setTimer();
      if (hdrCount >= 3)
      {
        SameMsgRcvd();
        // clear SAME buffer, disable timer, HDR_COUNT = 0
        timerExpiredMsgRcvd();
      }
    }
    command_sameint_clear();
  }
  
  if (asq_int) // audio signal quality interrupt
  {
    byte alertint;
    boolean alertnow;
    result = command_asq_status(0, &alertint, &alertnow);
    if (alertnow || MASK_BYTE(alertint, 0x1))
    {
      digitalWrite(ledPin, 1);
      setMuteVolume(false);
      setVolume(MAX_VOLUME);  // turn on speaker
    }
    else if (!alertnow && MASK_BYTE(alertint, 0x2))
    {
      digitalWrite(ledPin, 0);
      // Alert tone went away
      setMuteVolume(muted);
      setVolume(userVol);
    }
    else
    {
      digitalWrite(ledPin, 0);
    }
    command_asqint_clear();
  }
  
  if (stc_int) // seek/tune complete
  {
    if (seekTuneActive)
    {
      seekTuneActive = false;
      unsigned int rv = command_Tune_Status(0, 4);
      command_stcint_clear();
      
      Serial.print(F("Tune complete.\nRSSI = "));
      Serial.println(rv >> 8);
      Serial.print(F("SNR = "));
      Serial.println(rv & 0xff);
    }
  }
}

/// \brief Called whenever serial data is available
void serialEvent()
{
    while (Serial.available())
    {
        char c = Serial.read();
          // Once received, act on the serial input:
        switch (c)
        {
        case 'u':
            tuneWBFrequency(1);  // Tune up 1 increment (2.5kHz)
            break;
        case 'd':
            tuneWBFrequency(-1);  // Tune down 1 increment (2.5kHz)
            break;
        case 'U':
            tuneWBFrequency(10);  // Tune up 10 increments (25kHz)
            break;
        case 'D':
            tuneWBFrequency(-10);  // Tune down 10 increments (25kHz)
            break;
        case 's':
            printSAMEStatus();
            setVolume(userVol); // set volume back to user's volume
            setMuteVolume(muted);
            break;
        case 'r':
            Serial.print("RSSI = ");
            Serial.println(getRSSI());
            break;
        case 'S':
            Serial.print("SNR = ");
            Serial.println(getSNR());
            break;
        case 'o':
            Serial.print("Frequency offset = ");
            Serial.println(getFreqOffset());
            break;
        case 'f':
            Serial.print("Frequency = ");
            // getWBFrequency() returns the 2-byte frequency code sent
            // to the Si4707. To get a real-looking freq, multiply by .0025
            Serial.print((float) getWBFrequency() * 0.0025, 4);
            Serial.println(" MHz");
            break;
        case 'm':
            if (muted) Serial.println(F("Unmuting"));
            else Serial.println(F("Muting"));
            muted = !muted;
            setMuteVolume(muted);
            break;
        case '+':
            setVolume(++userVol); // increment volume
            Serial.print(F("Volume "));
            Serial.print(userVol);
            Serial.println(F("/63"));
            break;
        case '-':
            setVolume(--userVol); // decrement volume
            Serial.print(F("Volume "));
            Serial.print(userVol);
            Serial.println(F("/63"));
            break;
        case 'k':
            searchWBFrequency();
            Serial.print(F("Found best freq = "));
            Serial.print((float)getWBFrequency() * 0.0025, 4);
            Serial.println(F("MHz"));
        break;
        case 'h':
            printMenu(); // print the help menu
        }
    }   
}

void loop()
{
  if (interrupt)
  {
    SameEvent();
    interrupt = false;
  }

  if (timerAlarm)
  {
    timerAlarm = false;
    blinkVal++;
    digitalWrite(ledPin, blinkVal % 2);
    
    timerVal++;
    if (timerVal == 6)
    {
      Serial.println(F("SAME Timer expired"));
      // we need to clear the SAME buffer forefully
      // disable the same timer expiration for now
      timerExpiredMsgRcvd();
      
      blinkVal=0;
      digitalWrite(ledPin, 0);
    }
  }
  delay(10);
}

void printMenu()
{
  Serial.println(F("Weather Radio Menu Options:"));
  Serial.println(F("\t u) Tune +2.5kHz"));
  Serial.println(F("\t d) Tune -2.5kHz"));
  Serial.println(F("\t U) Tune +25kHz"));
  Serial.println(F("\t D) Tune -25kHz"));
  Serial.println(F("\t s) SAME status"));
  Serial.println(F("\t r) RSSI"));
  Serial.println(F("\t S) SNR"));
  Serial.println(F("\t f) Tune Frequency"));
  Serial.println(F("\t m) Toggle Mute"));
  Serial.println(F("\t +) Increase Volume"));
  Serial.println(F("\t -) Decrease Volume"));
  Serial.println(F("\t k) Seek best frequency"));
  Serial.println(F("\t h) Help"));
  Serial.println();
}

// initSi4707 performs the following functions, in sequence:
//  * Initialize all pins connected to Si4707 (SEN, SCLK, SDIO, and RST)
//  * Starts up the Wire class - used for two-wire communication
//  * Applies a reset signal to the Si4707
//  * Sends the Power Up command to the Si4707
//  * Sends the GET REV command to verify communication, returns the final
//    two digits of the Part Number (should be 7)
byte initSi4707()
{
  // Set initial pin value: RST (Active-low reset)
  pinMode(rstPin, OUTPUT);  // Reset
  digitalWrite(rstPin, LOW);  // Keep the SI4707 in reset
  
  // Set initial pin value: SEN (I2C Address select)
  pinMode(senPin, OUTPUT);  // Serial enable
  if (SEN_ADDRESS)
    digitalWrite(senPin, HIGH);
  else 
    digitalWrite(senPin, LOW);
  
  // Set initial pin values: SDIO and SCLK (serial data and clock lines)
  // Wire.begin() will take care of this
  Wire.begin();
  delay(1);  // Short delay before we take reset up
  
  // Raise RST, SCLK must not rise within 300ns before RST rises
  digitalWrite(rstPin, HIGH);
  delay(1);  // Give Si4707 a little time to reset
  
  // First, send the POWER UP command to turn on the Si4707.
  // The Si4707 must be powered up before sending any further commands.
  powerUp();

  // See Si4707_system_functions.ino for info on command_Get_Rev
  return command_Get_Rev(1);    
}

// Ths function sets the Si4707 to the freq it receives.
// freq should represent the frequency desired in kHz.
// e.g. 162400 will tune the radio to 162.400 MHz.
boolean setWBFrequency(long freq)
{
  // Keep tuned frequency between valid limits (162.4 - 162.55 MHz)
  long freqKhz = constrain(freq, 162400, 162550);
  
  // See si4707_system_functions.ino for info on command_Tune_Freq
  command_Tune_Freq((freqKhz * 10)/25);
}

void searchWBFrequency()
{
  long freqKhz = 162400;
  byte snr = 0;
  byte maxSNR = 0;
  long bestFreq = 162400;
  if (setWBFrequency(freqKhz))
  {
    snr = getSNR();
    maxSNR = snr;
    bestFreq = freqKhz;
  }
 
  // change the search range depending on received signal strength.
  const signed char badQualityInc = 5;
  const signed char goodQualityInc = 1;
  const signed char avgQualityInc = 2;

  signed char incr = badQualityInc;
  while (freqKhz < 162550)
  {
    if (snr >= 30) incr = goodQualityInc;
    if (snr <= 5) incr = badQualityInc;
    else incr = avgQualityInc;
    
    freqKhz += incr;
    setWBFrequency(freqKhz);
    snr = getSNR();
    
    if (snr > maxSNR)
    {
      bestFreq = freqKhz;
      maxSNR = snr;
    }
  }
  
  setWBFrequency(bestFreq);
}

// This function does incremental tunes on the Si4707. Send a
// signed value representing how many increments you'd like to go
// (up is positive, down is negative). Each increment is 2.5kHz.
void tuneWBFrequency(signed char increment)
{
  unsigned int freq = getWBFrequency();
  freq += increment;
  freq = constrain(freq, 64960, 65020);
    
  Serial.print(F("Tuning to: "));
  Serial.print((float) freq * 0.0025, 4);
  Serial.println(F(" MHz"));
  
  // See si4707_system_functions.ino for info on command_Tune_Freq
  command_Tune_Freq(freq);
}

// Returns the two-byte Si4707 representation of the frequency.
unsigned int getWBFrequency()
{
  // See si4707_system_functions.ino for info on command_Tune_Status
  return command_Tune_Status(0, 2);
}

// Returns the recieved signal strength reported by Si4707
byte getRSSI()
{
  // See si4707_system_functions.ino for info on command_RSQ_Status
  return command_RSQ_Status(0, 4);
}

// Returns the signal-to-noise ratio reported by the Si4707
byte getSNR()
{
  // See si4707_system_functions.ino for info on command_RSQ_Status
  return command_RSQ_Status(0, 5);
}

// Returns the frequency offset reported by the Si4707
signed char getFreqOffset()
{
  // See si4707_system_functions.ino for info on command_RSQ_Status
  return (signed char) command_RSQ_Status(0, 7);
}

// Prints the status value of the SAME status command
// Work still do be done on this function. (Haven't had a WSW recently!)
void printSAMEStatus()
{
  // we just poll the SAME status, but don't clear the buffer, this needs to be handled in the main logic loop.
  byte sameStatus = command_SAME_Status(2, 0, 0);
  if (!sameStatus)
    Serial.println(F("No SAME message detected"));
  else
  {
    Serial.print(F("SAME status: "));
    Serial.println(sameStatus);
    Serial.print(F("Message length: "));
    Serial.println(command_SAME_Status(3, 0, 0));
  }
}

// Depending on the value of the mute boolean, this function will either
//  mute (mute=1) or un-mute (mute=0) the Si4707's audio output.
void setMuteVolume(boolean mute)
{
  // Mute left (bit 1) and right (bit 0) channels
  setProperty(PROPERTY_RX_HARD_MUTE, (mute<<1) | (mute<<0));
}

// This functionn interacts with the RX_VOLUME property of the Si4707.
//  Send a volume value (vol) between 0 (mute) and 63 (max volume).
void setVolume(int vol)
{
  // the program can temporarily override the volume, which is why we set to vol
  // arg not userVol, which is more of the user desired volume
  vol = constrain(vol, 0, 63); // vol should be between 0 and 63
  userVol = constrain(userVol, 0, 63); // constrain the saved volume as well
  setProperty(PROPERTY_RX_VOLUME, vol);
}
