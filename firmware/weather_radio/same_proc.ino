#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#define MAX_SAME_MSG_SIZE 268
char sameMsg[MAX_SAME_MSG_SIZE]; // large enough to fit maximum size same msg
int sameMsgLen = 0;

// SAME msgs are sent in 3 

typedef struct
{
    char code[6];    
} FIPS_Loc;

typedef struct
{
    byte count;
    FIPS_Loc locations[10];
} FIPS_Loc_Settings;

FIPS_Loc_Settings __attribute__((section(".eeprom"))) area_settings;

typedef struct
{
    const char* code; // 3 byte transmitted event code
    const char* PROGMEM description; // human-readable description
    boolean active; // wether user cares about event  
} SAME_Event;

struct SAME_Event_List
{
    SAME_Event events[54];  
};

// Madison County, AL FIP 6-4 code
const char madison_fc[] PROGMEM = "001089";

const char bzwc[] PROGMEM = "BZW";
const char cfac[] PROGMEM = "CFA";
const char cfwc[] PROGMEM = "CFW";
const char dswc[] PROGMEM = "DSW";
const char ffac[] PROGMEM = "FFA";
const char ffwc[] PROGMEM = "FFW";
const char ffsc[] PROGMEM = "FFS";
const char flac[] PROGMEM = "FLA";
const char flwc[] PROGMEM = "FLW";
const char flsc[] PROGMEM = "FLS";
const char hwac[] PROGMEM = "HWA";
const char hwwc[] PROGMEM = "HWW";
const char huac[] PROGMEM = "HUA";
const char huwc[] PROGMEM = "HUW";
const char husc[] PROGMEM = "HLS";
const char svac[] PROGMEM = "SVA";
const char svrc[] PROGMEM = "SVR";
const char svsc[] PROGMEM = "SVS";
const char smwc[] PROGMEM = "SMW";
const char spsc[] PROGMEM = "SPS";
const char toac[] PROGMEM = "TOA";
const char torc[] PROGMEM = "TOR";
const char trac[] PROGMEM = "TRA";
const char trwc[] PROGMEM = "TRW";
const char tsac[] PROGMEM = "TSA";
const char tswc[] PROGMEM = "TSW";
const char wsac[] PROGMEM = "WSA";
const char wswc[] PROGMEM = "WSW";
const char eanc[] PROGMEM = "EAN";
const char eatc[] PROGMEM = "EAT";
const char nicc[] PROGMEM = "NIC";
const char nptc[] PROGMEM = "NPT";
const char rmtc[] PROGMEM = "RMT";
const char rwtc[] PROGMEM = "RWT";
const char adrc[] PROGMEM = "ADR";
const char avac[] PROGMEM = "AVA";
const char avwc[] PROGMEM = "AVW";
const char caec[] PROGMEM = "CAE";
const char cdwc[] PROGMEM = "CDW";
const char cemc[] PROGMEM = "CEM";
const char eqwc[] PROGMEM = "EQW";
const char evic[] PROGMEM = "EVI";
const char frwc[] PROGMEM = "FRW";
const char hmwc[] PROGMEM = "HMW";
const char lewc[] PROGMEM = "LEW";
const char laec[] PROGMEM = "LAE";
const char toec[] PROGMEM = "TOE";
const char nuwc[] PROGMEM = "NUW";
const char rhwc[] PROGMEM = "RHW";
const char spwc[] PROGMEM = "SPW";
const char vowc[] PROGMEM = "VOW";
const char nmnc[] PROGMEM = "NMN";
const char dmoc[] PROGMEM = "DMO";
const char xxxc[] PROGMEM = "XXX";

const char bzwd[] PROGMEM = "Blizzard Warning";
const char cfad[] PROGMEM = "Coastal Flood Watch";
const char cfwd[] PROGMEM = "Coastal Flood Warning";
const char dswd[] PROGMEM = "Dust Storm Warning";
const char ffad[] PROGMEM = "Flash Flood Watch";
const char ffwd[] PROGMEM = "Flash Flood Warning";
const char ffsd[] PROGMEM = "Flash Flood Statement";
const char flad[] PROGMEM = "Flood Watch";
const char flwd[] PROGMEM = "Flood Warning";
const char flsd[] PROGMEM = "Flood Statement";
const char hwad[] PROGMEM = "High Wind Watch";
const char hwwd[] PROGMEM = "High Wind Warning";
const char huad[] PROGMEM = "Hurricane Watch";
const char huwd[] PROGMEM = "Hurricane Warning";
const char husd[] PROGMEM = "Hurricane Statement";
const char svad[] PROGMEM = "Severe Thunderstorm Watch";
const char svrd[] PROGMEM = "Severe Thunderstorm Warning";
const char svsd[] PROGMEM = "Severe Weather Statement";
const char smwd[] PROGMEM = "Special Marine Warning";
const char spsd[] PROGMEM = "Special Weather Statement";
const char toad[] PROGMEM = "Tornado Watch";
const char tord[] PROGMEM = "Tornado Warning";
const char trad[] PROGMEM = "Tropical Storm Watch";
const char trwd[] PROGMEM = "Tropical Storm Warning";
const char tsad[] PROGMEM = "Tsunami Watch";
const char tswd[] PROGMEM = "Tsunami Warning";
const char wsad[] PROGMEM = "Winter Storm Watch";
const char wswd[] PROGMEM = "Winter Storm Warning";
const char eand[] PROGMEM = "Emergency Action Notification";
const char eatd[] PROGMEM = "Emergency Action Termination";
const char nicd[] PROGMEM = "National Information Center";
const char nptd[] PROGMEM = "National Periodic Test";
const char rmtd[] PROGMEM = "Required Monthly Test";
const char rwtd[] PROGMEM = "Required Weekly Test";
const char adrd[] PROGMEM = "Administrative Message";
const char avad[] PROGMEM = "Avalanche Watch";
const char avwd[] PROGMEM = "Avalanche Warning";
const char caed[] PROGMEM = "Child Abduction Emergency";
const char cdwd[] PROGMEM = "Civil Danger Warning";
const char cemd[] PROGMEM = "Civil Emergency Message";
const char eqwd[] PROGMEM = "Earthquake Warning";
const char evid[] PROGMEM = "Evacuation Immediate";
const char frwd[] PROGMEM = "Fire Warning";
const char hmwd[] PROGMEM = "Hazardous Materials Warning";
const char lewd[] PROGMEM = "Law Enforcement Warning";
const char laed[] PROGMEM = "Local Area Emergency";
const char toed[] PROGMEM = "911 Telephone Outage Emergency";
const char nuwd[] PROGMEM = "Nuclear Power Plant Warning";
const char rhwd[] PROGMEM = "Radiological Hazard Warning";
const char spwd[] PROGMEM = "Shelter In Place Warning";
const char vowd[] PROGMEM = "Volcano Warning";
const char nmnd[] PROGMEM = "Network Message Notification";
const char dmod[] PROGMEM = "Practice/Demo Warning";
const char xxxd[] PROGMEM = "Unknown EAS Event Code!";

const SAME_Event_List same_events PROGMEM = {
{
{bzwc, bzwd, true},
{cfwc, cfwd, true},
{cfac, cfad, true},
{dswc, dswd, true},
{ffac, ffad, true},
{ffwc, ffwd, true},
{ffsc, ffsd, true},
{flac, flad, true},
{flwc, flwd, true},
{flsc, flsd, true},
{hwac, hwad, true},
{hwwc, hwwd, true},
{huac, huad, true},
{huwc, huwd, true},
{husc, husd, true},
{svac, svad, true},
{svrc, svrd, true},
{svsc, svsd, true},
{smwc, smwd, true},
{spsc, spsd, true},
{toac, toad, true},
{torc, tord, true},
{trac, trad, true},
{trwc, trwd, true},
{tsac, tsad, true},
{tswc, tswd, true},
{wsac, wsad, true},
{wswc, wswd, true},
{eanc, eand, true},
{eatc, eatd, true},
{nicc, nicd, true},
{nptc, nptd, true},
{rmtc, rmtd, true},
{rwtc, rwtd, true},
{adrc, adrd, true},
{avac, avad, true},
{avwc, avwd, true},
{caec, caed, true},
{cdwc, cdwd, true},
{cemc, cemd, true},
{eqwc, eqwd, true},
{evic, evid, true},
{frwc, frwd, true},
{hmwc, hmwd, true},
{lewc, lewd, true},
{laec, laed, true},
{toec, toed, true},
{nuwc, nuwd, true},
{rhwc, rhwd, true},
{spwc, spwd, true},
{vowc, vowd, true},
{nmnc, nmnd, true},
{dmoc, dmod, true},
{xxxc, xxxd, true}
}
};

boolean is_printable(char c)
{
    if ((int)c < 32) return false;
    return true;
}

void print_char(char c)
{
    if (is_printable(c))
    {
        Serial.print(c);
    }
    else
    {
        Serial.print("\\x");
        Serial.print(c, HEX);   
    }
}

void printSameMsg()
{
    for (int i = 0; i < sameMsgLen; i++)
    {
        print_char(sameMsg[i]);
    }
    Serial.println("");
    
    /*
    for (int i = 0; i < sameMsgLen; i++)
    {
        Serial.print("DATA");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(sameMsg[i], HEX);
    }
    */
}

// todo need a way to determine watch, warning, statement, etc...
const char* same_description(const char* evcode, boolean* enabled)
{
    static char s_descr[31]; // we have to copy the description from flash
    byte i = 0;
    const char* ptr;
    while (true)
    {
        ptr = (const char*) pgm_read_word(&same_events.events[i].code);
        // check for the sentinel
        if (memcmp_P("XXX", ptr, 3) == 0) break;
        // check for a match
        if (memcmp_P(evcode, ptr, 3) == 0) break;
        i++;
    }

    if (enabled) *enabled = pgm_read_word(&same_events.events[i].active);
    ptr = (const char*) pgm_read_word(&same_events.events[i].description);
    strcpy_P(s_descr, ptr);
    return s_descr;
}

// SAME msg format:
// (Preamble) ZCZC-ORG-EEE-PSSCCC-PSSCCC+TTTT-JJJHHMM-LLLLLLLL-
// (1 sec pause)
// (Preamble) ZCZC-ORG-EEE-PSSCCC-PSSCCC+TTTT-JJJHHMM-LLLLLLLL-
// (1 sec pause)
// (Preamble) ZCZC-ORG-EEE-PSSCCC-PSSCCC+TTTT-JJJHHMM-LLLLLLLL-
// (1 - 3 sec pause) 
// 1050 Hz Warning Alarm for 8-10 sec (if transmitted)
// (3 - 5 sec pause)
// Computer voice generated message (if transmitted)
// (1 - 3 sec pause)
// (Preamble) NNNN
// (1 sec pause)
// (Preamble) NNNN
// (1 sec pause)
// (Preamble) NNNN
boolean recvSameMsg()
{
    byte offset = 0;
    sameMsgLen = command_SAME_Data(offset, sameMsg);
    Serial.print(F("Receiving "));
    Serial.print(sameMsgLen);
    Serial.println(F(" byte SAME msg."));
    
    byte n = sameMsgLen - 8;
    if (sameMsgLen <= 8)
    {
        goto same_rcv_suc;
    }
    
    offset += 8;
    
    if (sameMsgLen > MAX_SAME_MSG_SIZE)
    {
        goto same_rcv_err;
    }
  
    while (n >= 8)
    {
        command_SAME_Data(offset, &sameMsg[offset]);
        n -= 8;
        offset += 8;
    }
    
    if (n > 0)
    {
        // If the message is > 264 bytes we need a partial read to 
        // fit the last n bytes of data. We don't want to write
        // off the end of our sameMsg buffer.
        char temp[8];
        command_SAME_Data(offset, temp);
        memcpy(&sameMsg[offset], temp, n);
    }

same_rcv_suc:
    return true;

same_rcv_err:
    return false;
}

boolean acceptLocCode(const char* loc)
{
    byte i, num;
    int offset = 0;
    num = eeprom_read_byte(&area_settings.count);
    offset++;
    char saved_code[6];
    
    for (i = 0; i < num; i++)
    {
        eeprom_read_block(saved_code, area_settings.locations[i].code, 6);
        if (loc[0] != saved_code[0]) goto mismatch;
        if (loc[1] != saved_code[1]) goto mismatch;
        if (loc[2] != saved_code[2]) goto mismatch;
        if (loc[3] != saved_code[3]) goto mismatch;
        if (loc[4] != saved_code[4]) goto mismatch;
        if (loc[5] != saved_code[5]) goto mismatch;  
        return true;
        
        mismatch:
        offset+=6;
    }
    return false;
}

// this function will receive and parse out a same string
boolean processSameMsg()
{
    // The si4707 uses the preamble and ZCZC header to lock the signal
    // and doesn't actually hand that to use as part of the SAME data request.
    // We will see a '-' or an 'N' in byte 0.
    const char* buf = sameMsg;

    char org[3]; // originator code
    char evCode[3]; // event header code block
    char purge[4]; // purge time, how long the msg is valid from now
    char calTime[7]; // julian calendar date msg was sent
    char orig[8]; // msg originator
    boolean err_fmt = false;
    if (buf[0] != '-') err_fmt = true; // invalid format

    memcpy(org, &buf[1], 3);
    if (buf[4] != '-') err_fmt = true; // invalid format
    memcpy(evCode, &buf[5], 3);
    
    boolean proceed = true;
    const char* description = same_description(evCode, &proceed);
    
    if (!proceed) return false; // user not interested in this message type
    Serial.print(F("Processing "));
    Serial.println(description);

    // \todo check the event code, and see if we want to keep the msg
    const char* locCode;
    // read the location codes
    buf += 8;
    boolean filterLoc = true;
    while (true)
    {
        // we're reading 8 byte chunks and expecting to see
        // -AAAAAA-d.
        // where A is an ascii character, - is a '-' and d is don't care
        // note that the last location code will have a + in the 7th byte
        // position
        locCode = buf;
        if (locCode[0] != '-') err_fmt = true; // invalid format
        locCode++;
        // \todo buf[1:6] contain a position code -- do something with it
        if (acceptLocCode(locCode)) filterLoc = false;
        // look for the end of the positions
        if (buf[7] == '+') break;
        else if (buf[7] != '-') err_fmt = true;
        buf += 7;
    }
    buf += 7;
    
    // the byte at position offset is a '+'
    if (buf[0] != '+') err_fmt = true;
    memcpy(purge, &buf[1], 4);

    if (buf[5] != '-') err_fmt = true;
    buf += 6;

    // buffer contains JJJHHMM
    memcpy(calTime, buf, 7);
    if (buf[7] != '-') err_fmt = true;
    buf += 8;

    memcpy(orig, buf, 8);
    // note -- sometimes we get some extra garbage data at the end
    // of the transmission, we just ignore any data after the msg
    // originator string.
    if (err_fmt == true)
    {
      Serial.println(F("detected some errors in msg format."));
    }
    
    if (filterLoc)
    {
        Serial.println(F("not applicable to select area code(s)"));   
    }
    return true;
}

// used to verify that a equals b, for the SAME self-test
// note that b must point to a string in flash memory
// If the test fails, the led will blink at 1Hz indefinitely
void verifyStr(const char* a, const char* b)
{
    if (!strcmp_P(a, b))
    {
        return;   
    }
    
    for (;;)
    {
        digitalWrite(ledPin, 1);
        delay(1000);
        digitalWrite(ledPin, 0);
        delay(1000);
    }
}

void sameInit()
{
    char def_code[6];
    memcpy_P(def_code, madison_fc, 6); // add our county to the list of area codes to process
    eeprom_update_byte(&area_settings.count, 2); // first byte indicates number of saved codes (max 10)
    eeprom_update_block(def_code, area_settings.locations[0].code, 6);
}

void sameTest()
{
    int count = 0;
    // make sure the sentinel works, if not we'll see an led light up forever,
    // we'll be in an infinite loop, maybe
    Serial.println(F("Starting Test"));
    digitalWrite(ledPin, 1);
    delay(500);
    verifyStr(same_description("BZW", 0), PSTR("Blizzard Warning"));
    verifyStr(same_description("ABC", 0), PSTR("Unknown EAS Event Code!"));
    digitalWrite(ledPin, 0);
    
    /// \todo remove this test
    strcpy_P(sameMsg, PSTR("-WXR-RWT-047103-047055-001079-001083-001089-001071-001095-001103+0600-1411643-KHUN/NWS-"));
    if (processSameMsg())
    {
        Serial.println(F("Success"));
        count = 3;
    }
    else
    {
        Serial.println(F("Error"));
        count = 10;
    }
    
    for (int i = 0; i < count; i++)
    {
        digitalWrite(ledPin, 0);
        delay(500);
        digitalWrite(ledPin, 1);
        delay(500);   
    }
    digitalWrite(ledPin, 0);
}
