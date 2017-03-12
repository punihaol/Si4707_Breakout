/* Si4707_system_functions contains the nitty gritty system
   functions used to interface with the Si4707. High-level stuff,
   like constructing command strings, and parsing response strings.
   Also functions like sending commands, and waiting for the
   clear-to-send bit to be set.
   
   There are prettier wrappers for most of these functions in
   the example sketch.
   
   At the bottom of this sketch are functions used to interface
   with the Wire library - i2cWriteBytes and i2cReadBytes. */
 
//////////////////////////////
// High level functions //////
//////////////////////////////

// These arrays will be used by most command functions to construct
// the command string and receive response bytes.
byte rsp[15];
byte cmd[8];

/* powerup(): Sends the POWER_UP command to initiate the boot 
   process, moving the Si4707 from powerdown to powerup mode.
   
   This performs critical operations in configuring the Si4707
   crystal oscillator, and turning on WB receive. */
void powerUp(void)
{
  /* Power up (0x01) - initiate boot process. 
     2 Arguments:
       ARG1: (CTSIEN)(GPO2OEN)(PATCH)(XOSCEN)(FUNC[3:0])
       ARG2: (OPMODE[7:0])  
     Response: none (if FUNC = 3)*/
  cmd[0] = COMMAND_POWER_UP;  // Command 0x01: Power Up
  cmd[1] = 0x53;  // GP02 output enabled, crystal osc enabled, WB receive mode
  cmd[2] = 0x05;  // Use analog audio outputs
  writeCommand(3, cmd, 0, rsp);
  
  delay(POWER_UP_TIME_MS);
}

/* command_Tune_Freq(uint16_t frequency) sends the WB_TUNE_FREQ command
  with freqeuncy as the arguments. After tuning, the WB_TUNE_STATUS
  command is sent to verify whether or not tuning was succesful.
  
  frequency should be a 16-bit value equal to the freq. you want to
  tune to divided by 2.5kHz.  
  For example, for 162.4MHz send 64960 (162400000 / 2500 = 64960)
  For 162.55 MHz send 65020 (162550000 / 2500 = 65020). 
  
  Return RESP1 byte of WB_TUNE_STATUS response (valid and afcrl bits) */
void command_Tune_Freq(unsigned int frequency)
{
  cmd[0] = COMMAND_WB_TUNE_FREQ;
  cmd[1] = 0;
  cmd[2] = (uint8_t)(frequency >> 8);
  cmd[3] = (uint8_t)(frequency & 0x00FF);
  seekTuneActive = true;
  writeCommand(4, cmd, 1, rsp);

  byte i = 0;
  while (true)
  {
    byte rv = command_Get_Int_Status(0, 0);
    if (MASK_BYTE(rv, STCINT_MASK))
    {
      //STCINT was set, the chip is done tuning, return early
      command_stcint_clear();
      command_asqint_clear(); // bug -- discard spurious asqint that sometimes happens after tune
                              // note -- I don't think we can prevent this here, may need to catch
                              // it in the interrupt handler somehow
      return;
    }
    delay(25);
    i++;
    if (i >= 20) break;
  }

  //Tuning error, waiting maximum time as failsafe
  delay(500);
  command_stcint_clear();
}

/* WB_TUNE_STATUS (0x52) - Returns current frequency and RSSI/SNR
  at the moment of tune.
   Arguments (1 byte):
     (1) ARG1 - bit 0: INTACK (if set, clears seek/tune complete int)
   Response (6 bytes): 
     (1) Status
     (2) bit 1: AFC rail indicator
         bit 0: Valid channel indicator
     (3) READFREQ(H) - Read frequency high byte
     (4) READFREQ(L) - Read frequency low byte
     (5) RSSI - Receive signal strength at frequency
     (6) SNR - Signal-to-noise ratio at frequency */  
unsigned int command_Tune_Status(byte intAck, byte returnIndex)
{
  cmd[0] = COMMAND_WB_TUNE_STATUS;
  cmd[1] = intAck & 1;
  writeCommand(2, cmd, 6, rsp);
  
  return ((rsp[returnIndex] << 8) | rsp[returnIndex + 1]);
}

void command_stcint_clear()
{
  unsigned int stat = command_Tune_Status(1,0);
  while (MASK_BYTE((stat >> 8), STCINT_MASK))
  {
    stat = command_Tune_Status(1, 0);
  } 
}
/* command_Get_Rev() sends the GET_REV command   
   Response (9 bytes): 
     (0) Status (Should have CTS bit (7) set)
     (1) PN[7:0] - Final two digits of part number 
     (2) FWMAJOR[7:0] - Firmware major revision
     (3) FWMINOR[7:0] - Firmware minor revision
     (4) PATCH(H) - Patch ID high byte
     (5) PATCH(L) - Patch ID low byte
     (6) CMPMAJOR[7:0] - Component major revision
     (7) CMPMINOR[7:0] - Component minor revision
     (8) CHIPREV[7:0] - Chip revision
     
   Returns the response byte requested */
byte command_Get_Rev(byte returnIndex)
{
  cmd[0] = COMMAND_GET_REV;
  writeCommand(1, cmd, 9, rsp);
  
  return rsp[returnIndex];
}

byte command_asq_status(byte intack, byte* alertint, boolean* alertnow)
{
  cmd[0] = COMMAND_WB_ASQ_STATUS;
  cmd[1] = intack & 0x1;
  writeCommand(2, cmd, 3, rsp);
  if (alertint) *alertint = rsp[1];
  if (alertnow) *alertnow = MASK_BYTE(rsp[2], 0x01);
  return rsp[0];
}

void command_asqint_clear()
{
  byte stat = command_asq_status(1, NULL, NULL);
  while (MASK_BYTE(stat, ASQINT_MASK))
  {
    stat = command_asq_status(1, NULL, NULL);
  }
}

/* command_SAME_Status() sends the WB_SAME_STATUS command and
  returns the requested byte in the response
  
  Arguments (2 bytes):
    (0): ()()()()()()(CLRBUF)(INTACK)
    (1): READADDR[7:0]
   Response (13 bytes):
    (0): Status byte
    (1): ()()()()(EMODET)(SOMDET)(PREDET)(HDRRDY)
    (2): STATE[7:0]
    (3): MSGLEN[7:0]
    (4): Confidence of data bytes 7-4
    (5): Confidence of data bytes 3-0
    (6-13): DATA0, DATA1, ..., DATA7
    
    The request allows us to clear the message buffer and acknowledge the
    SAME interrupt.
    The SAME message buffer must be cleared if one of the following conditions
    is met:
    1) Finished receipt of end-of-message
    2) Finished receipt of 3 headers
    3) 1050 Hz alert tone was received
    4) 6 seconds have passed since the completion of the reception of the last
        header, and no new preamble was detected.
*/
byte command_SAME_Status(byte returnIndex, byte intAck, byte clrBuf)
{
  clrBuf &= 0x1;
  intAck &= 0x1;
  
  cmd[0] = COMMAND_WB_SAME_STATUS;
  cmd[1] = (clrBuf << 1) | (intAck);
  cmd[2] = 0;
  
  writeCommand(3, cmd, 14, rsp);
  
  return rsp[returnIndex];
}

void command_sameint_clear()
{
  byte stat = command_SAME_Status(0, 1, 0);
  while (MASK_BYTE(stat, SAMEINT_MASK))
  {
    stat = command_SAME_Status(0, 1, 0); 
  }
}

byte command_SAME_Data(byte offset, char* buffer)
{
    boolean cts = true;
    boolean err = false;
    
    // not sure about this... i think since we poll the WB_SAME_STATUS
    // in a loop as fast as possible, sometimes its not ready for us??
    while (true)
    {
        command_Get_Int_Status(&cts, &err);
        if (err)
        {
            Serial.println(F("Detected error calling WB_SAME_STATUS:GET_INT_STATUS"));
        }
        
        if (cts) break;
        delay(50); // arbitrarily chosen
    }    
    
    cmd[0] = COMMAND_WB_SAME_STATUS;
    cmd[1] = 0;
    cmd[2] = offset;
  
    writeCommand(3, cmd, 14, rsp);
    err = (rsp[0] >> 6) & 0x1;
    if (err)
    {
        Serial.println(F("Detected error calling WB_SAME_STATUS"));
    }
  
    buffer[0] = (char)rsp[6];
    buffer[1] = (char)rsp[7];
    buffer[2] = (char)rsp[8];
    buffer[3] = (char)rsp[9];
    buffer[4] = (char)rsp[10];
    buffer[5] = (char)rsp[11];
    buffer[6] = (char)rsp[12];
    buffer[7] = (char)rsp[13];
  
    return rsp[3]; //return the total msg len
}
/* WB_RSQ_STATUS (0x53) - Returns status information about 
  received signal quality - RSSI, SNR, freq offset.
   Argument (1 byte):
     (0) bit 0: Interrupt acknowledge
   Response (8 bytes): 
     (0) Status (Should have CTS bit (7) set)
     (1) RESP1: ()()()()(SNRHINT)(SNRLINT)(RSSIHINT)(RSSILINT)
     (2) RESP2: ()()()()()()(AFCRL)(VALID)
     (3) Nothing
     (4) RSSI[7:0] - Received signal strength indicator
     (5) SNR[7:0] - Signal-to-noise metric
     (6) Nothing
     (7) FREQOFF[7:0] - signed frequency offset in kHz  */
byte command_RSQ_Status(byte intAck, byte returnIndex)
{

  cmd[0] = COMMAND_WB_RSQ_STATUS;
  cmd[1] = intAck & 1;
  
  writeCommand(2, cmd, 8, rsp);
  
  return rsp[returnIndex];
}

/* command_Get_Int_Status() sends the GET_INT_STATUS command and returns
  the byte from the commands response 
*/
byte command_Get_Int_Status(boolean* cts, boolean* err)
{
  cmd[0] = COMMAND_GET_INT_STATUS;
  cmd[1] = 0;
  rsp[1] = 0;
  
  writeCommand(1, cmd, 1, rsp);
  
  if (cts)
  {
      *cts = (rsp[0] >> 7) & 0x1;   
  }
  if (err)
  {
      *err = (rsp[0] >> 6) & 0x1; 
  }
  
  return rsp[0];
}

// This function will wait until the CTS (clear-to-send) bit has
// been set (or will timeout after 500 ms)
void waitForCTS(void)
{
  byte status = 0;
  int i = 1000;
  
  while (--i && !(status&0x80))
  {
    i2cReadBytes(1, &status);
    delayMicroseconds(500);
  }
}

// This function will write a command string with a given size
// And retrun a filled reply string with replySize bytes in it.
void writeCommand(byte cmdSize, byte * command, byte replySize, byte * reply)
{
  waitForCTS();
  i2cWriteBytes(cmdSize, command);
  waitForCTS();
  if (replySize)
  {
    i2cReadBytes(replySize, reply);
  }
  
  for (int i=0; i<replySize; i++)
  {
    reply[i] += 1;
    reply[i] -= 1;
  }
}

// This function returns a two-byte property from the requested
// property address.
unsigned int getProperty(unsigned int property)
{
  cmd[0] = COMMAND_GET_PROPERTY;
  cmd[1] = 0;
  cmd[2] = (byte)((property & 0xFF00) >> 8);
  cmd[3] = (byte)(property & 0x00FF);
  writeCommand(4, cmd, 4, rsp);
  
  return ((rsp[2] << 8) | rsp[3]);
}

// This function sets a property to the given two-byte property
//  value.
void setProperty(unsigned int propNumber, unsigned int propValue)
{
  cmd[0] = COMMAND_SET_PROPERTY;
  cmd[1] = 0;
  cmd[2] = (uint8_t)(propNumber >> 8);
  cmd[3] = (uint8_t)(propNumber & 0x00FF);
  cmd[4] = (uint8_t)(propValue >> 8);
  cmd[5] = (uint8_t)(propValue & 0x00FF);
  
  writeCommand(6, cmd, 1, rsp);  
}

//////////////////////////////
// Low level functions ///////
//////////////////////////////

// Read a specified number of bytes via the I2C bus.
// Will timeout if there is no response from the address.
void i2cReadBytes(byte number_bytes, byte * data_in)
{
  int timeout = 1000;
  
  Wire.requestFrom((byte) SI4707_ADDRESS, number_bytes);
  while ((Wire.available() < number_bytes) && (--timeout))
    ;
  while((number_bytes--) && timeout)
  {
    *data_in++ = Wire.read();
  }
  Wire.endTransmission();
}

// Write a specified number of bytes to the Si4707.
void i2cWriteBytes(uint8_t number_bytes, uint8_t *data_out)
{
  Wire.beginTransmission(SI4707_ADDRESS);
  while (number_bytes--)
  {
    Wire.write(*data_out++);
  }
  Wire.endTransmission();
}
