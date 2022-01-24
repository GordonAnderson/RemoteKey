/*
 * Local Controller firmware
 * 
 * KG7YU remote keyer system.
 * 
 * The remote keyer system is designed to allow the use of remote paddles, and keys (straight keys,
 * bugs, side-swipers) to be used to control an amateur radio transmitter. The system uses the 
 * internet to send messages from the remote location to the local location where the transmitter 
 * is located. Two controllers are used, a Remote controller connect to the key and the Local 
 * controller connected to your transmitter.
 * 
 * At the heart of both of these controllers are Adafruit Feather development boards. These development 
 * board are programmed using the Adruino development environment. This environment has broad community 
 * support allowing rapid development.
 * 
 * The Local controller uses as Adafruit M0 Feather development board and a Adafruit Ethernet 
 * featherwing. The featherwing adds a hardwire ethernet interface. The Local controller is directly 
 * connector to the internet access point near the transmitter. The Local controller accepts incoming 
 * connections from the Remote controller and processes key signals. The transmitter is keyed using 
 * an open collector output that connects to the transmitters key input. Below is a list of the Local 
 * controller functions:
 * 
 *    - Hardwire ethernet connection
 *    - Accepts UDP and TCP connections from the remote controller
 *    - Processes key up and down as well as paddle messages to key the transmitter
 *    - Link performance testing 
 *    - Fail safe max key down limits
 *    - Auxiliary control outputs
 *    - USB powered
 *    - USB host interface commands to configure and save settings
 *
 * The access point at the local location needs to be configured to forward the UDP and TCP ports to 
 * the IP address if the Local controller. The Local controller should also be assigned a fixed IP 
 * address. Commands in the Local controller allow you to define and save the IP address. This setup 
 * only needs to be done once. 
 * 
 * UDP message format
 *    D = key down
 *    U = key up
 *    . = generate a dit and space, dit function will block
 *    - = generate a dash and space, dash function will block
 *    W,xxx = speed in words per minute
 *    T,ddd,xxx = link test. ddd = message space in mS, xxx = sample size
 *    S,string = Send string, this function will block
 *    
 *  To do list:
 * 
 * Release history:
 * 
 * Gordon Anderson
 * gaa@owt.com
 * 509.628.6851
 * 
 */
#include <arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <stdio.h>
#include <string.h>
#include <arduino-timer.h>
#include "Morse.h"
#include "Serial.h"
#include "Errors.h"
#include "Local.h"
#include <FlashStorage.h>

LocalData ld;

LocalData Rev_1_ld = 
{
  sizeof(LocalData),
  "Local",
  1,
  // Ethernet parameters
  10,0,0,200,
  2015,
  2015,
  // Link test parameters
  false,0,0,0,0,0,
  // Keyer parameters
  19,
  SIGNATURE
};

// Reserve a portion of flash memory to store configuration
// Note: the area of flash memory reserved is lost every time
// the sketch is uploaded on the board.
FlashStorage(flash_ld, LocalData);

const char *Version = "KeyLocal Version 1.0, November 5, 2021";

auto timer = timer_create_default();

Morse morse;

unsigned long nowT;
unsigned long lastT;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x98, 0x76, 0xB6, 0x11, 0x5F, 0x0B };
IPAddress ip(10, 0, 0, 200);

// Initialize the Ethernet server library
// with the IP address and port you want to use
EthernetServer server(2015);
//EthernetServer *server;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];       // buffer to hold incoming packet
char ReplyBuffer[UDP_TX_PACKET_MAX_SIZE];        // buffer for outgoing messages

EthernetUDP Udp;

EthernetClient client;

// This function process all the serial IO and commands
void ProcessSerial(bool scan = true)
{
// Process data from tcp connection  
  if(client.connected()) 
  {
    if(client.available())
    {
      //serial->println(client.read());
      PutCh(client.read());
      serial = &client;
    }
  } else if(server.available()) client = server.available();
// Put serial received characters in the input ring buffer
  if (Serial.available() > 0)
  {
    PutCh(Serial.read());
    serial = &Serial;
  }
  if (!scan) return;
  // If there is a command in the input ring buffer, process it!
  if (RB_Commands(&RB) > 0) while (ProcessCommand() == 0); // Process until flag that there is nothing to do
}

/*
 * UDP message format
 *    D = key down
 *    U = key up
 *    . = generate a dit and space, dit function will block
 *    - = generate a dash and space, dash function will block
 *    W,xxx = speed in words per minute
 *    T,ddd,xxx = link test. ddd = message space in mS, xxx = sample size
 *    S,string = Send string, this function will block
 * 
 */
void ProcessUDP(char *buffer = NULL)
{
  char    *buf = NULL;
  uint8_t SeqNr;
  static  uint8_t LastSeqNr = 0;
  static  String token;
  static  int Ssize=0,Csample=0;
  static  float e;
  int num;

  buf = buffer;
  if(buf != NULL) num = strlen(buf);
  if(buf == NULL) if (num=Udp.parsePacket()) 
  {
    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    buf = packetBuffer;
  }
  if (buf != NULL) 
  {
    nowT = millis();
    switch (buf[0])
    {
      case 'D':
        if(num>=2)
        {
           if(LastSeqNr == buf[1]) break;
           morse.KeyDown();
           SeqNr = buf[1];
           if(++LastSeqNr != SeqNr) {Serial.print("SeqNr error D! "); Serial.println(SeqNr);} 
           LastSeqNr = SeqNr;
        }
        else morse.KeyDown();
        break;
      case 'U':
        if(num>=2)
        {
           if(LastSeqNr == buf[1]) break;
           morse.KeyUp();
           SeqNr = buf[1];
           if(++LastSeqNr != SeqNr) {Serial.print("SeqNr error U! "); Serial.println(SeqNr);}
           LastSeqNr = SeqNr;
        }
        else morse.KeyUp();
        break;
      case '.':
        if(num>=2)
        {
           if(LastSeqNr == buf[1]) break;
           morse.Dit();
           SeqNr = buf[1];
           if(++LastSeqNr != SeqNr) {Serial.print("SeqNr error Dit! "); Serial.println(SeqNr);}
           LastSeqNr = SeqNr;
        }
        else morse.Dit();
        break;
      case '-':
        if(num>=2)
        {
           if(LastSeqNr == buf[1]) break;
           morse.Dash();
           SeqNr = buf[1];
           if(++LastSeqNr != SeqNr) {Serial.print("SeqNr error Dash! "); Serial.println(SeqNr);}
           LastSeqNr = SeqNr;
        }
        else morse.Dash();
        break;
      case 'p':
        // Link keep alive, do nothing
        break;
      case 'W':
        // Get the token after the W, its the speed value
        token = GetToken(buf,2);
        if(token != "")
        {
          int i = token.toInt();
          if((i>=minWPM)&&(i<=maxWPM)) morse.wpm(ld.wpm = i);
        }
        break;
      case 'T': // Used for link testing, parameters are spacing in mS, sample size.
                // Sending T with no parameters will terminate a link test.
        token = GetToken(buf,2);
        if(token == "") { ld.linkTesting = false; break; }
        ld.mean = token.toFloat();
        token = GetToken(buf,3);
        if(token == "") break;
        Ssize = token.toInt();
        Csample = -2;
        ld.minError = ld.maxError = ld.sd = 0;
        ld.missedPackets = 0;
        ld.linkTesting = true;
       break;
      case 't': // Used for the link test, calculates the link performance, i.e. the following parameters
                // min error, max error, sd, missed packets. This message type is ignored if not test is in
                // process.
        if(~ld.linkTesting) break;
        if(Ssize == 0) break;
        if(Csample++ >= 0)
        {
          e = (nowT - lastT) - ld.mean;
          while(e > ld.mean) { ld.missedPackets++; e -= ld.mean; }
          if(e < ld.minError) ld.minError = e;
          if(e > ld.maxError) ld.maxError = e;
          ld.sd += e * e;
        }
        if(Ssize == Csample)
        {
          ld.sd = sqrt(e / Ssize);
          Ssize = Csample = 0;
          ld.linkTesting = false;
        }
        break;
      case 'S':
        // A comman should follow the S, if so send what remains
        if(buf[1] == ',') morse.SendMorseString(&buf[2]);
        break;
      default:
        serial->print(buf);
        serial->print(", ");
        serial->println(nowT - lastT);
        break;
    }
    lastT = nowT;
  }  
}

bool timerProcessSerial(void *)
{
  ProcessSerial();
  return true;
}

void setup()
{
  delay(100);
  // Read the flash config contents and test the signature
  ld = flash_ld.read();
  if(ld.Signature != SIGNATURE) ld = Rev_1_ld;
  // Init serial port
  SerialInit();
  //pinMode(13,OUTPUT);
  //digitalWrite(13,HIGH);
  ip = ld.IP;
  server = EthernetServer(ld.tcpPort);
  morse.begin(13,true);  
  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(10);  // Most Arduino shields
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) 
  {
    serial->println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  // UDP
  Udp.begin(ld.udpPort);
  // start the server
  server.begin();
  timer.every(10, timerProcessSerial);
}

// Main processing loop.
void loop(void)
{
  timer.tick();
  ProcessUDP();
  morse.check();
}

// Host commands

void Software_Reset(void)
{
  NVIC_SystemReset();    
}

void SaveSettings(void)
{
  ld.Signature = SIGNATURE;
  flash_ld.write(ld);
  SendACK;  
}

void RestoreSettings(void)
{
  static LocalData ldata;
  
  // Read the flash config contents and test the signature
  ldata = flash_ld.read();
  if(ldata.Signature == SIGNATURE) ld = ldata;
  else
  {
    SetErrorCode(ERR_EEPROMWRITE);
    SendNAK;
    return;
  }
  SendACK;    
}

void SetPortDir(char *port, char *mode)
{
  String Token;

  Token = port;
  int prt = Token.toInt();
  Token = mode;
  if(Token == "INPUT") pinMode(prt,INPUT_PULLUP);
  else if(Token == "OUTPUT") pinMode(prt,OUTPUT);
  else
  {
    SetErrorCode(ERR_BADARG);
    SendNAK;
    return;        
  }
  SendACK;
}

void SetPort(char *port, char *state)
{
  String Token;

  Token = port;
  int prt = Token.toInt();
  Token = state;
  if(Token == "HIGH") digitalWrite(prt,HIGH);
  else if(Token == "LOW") digitalWrite(prt,LOW);
  else
  {
    SetErrorCode(ERR_BADARG);
    SendNAK;
    return;        
  }
  SendACK;
}

void GetPort(int port)
{
  SendACKonly;
  if(digitalRead(port) == HIGH) serial->println("HIGH");
  else serial->println("LOW");
}

void SetWPM(int wpm)
{
  if((wpm < minWPM) || (wpm > maxWPM))
  {
    SetErrorCode(ERR_BADARG);
    SendNAK;
    return;    
  }
  morse.wpm(wpm);
  SendACK;
}

void String2upd(void)
{
  String mess;
  char   c;

  mess = "";
  while((c=GetCh()) != 0xFF) if(c == ',') break;
  while((c=GetCh()) != 0xFF)
  {
    if(c == '\n') break;
    mess += c;
  }
  mess.trim();
  ProcessUDP((char *)mess.c_str());
  SendACK;
}

void LinkStatus(void)
{
  SendACKonly;
  switch (Ethernet.linkStatus())
  {
    case LinkOFF:
      serial->println("LinkOFF");
      break;
    case LinkON:
      serial->println("LinkON");
      break;
    case Unknown:
      serial->println("Unknown");
      break;
  }
}

void ConnectStatus(void)
{
  SendACKonly;
  if(client.connected()) serial->println("Client connected!");
  else serial->println("No client connection!");
}

void SetIP(char *ipadd)
{
  if(ip.fromString(ipadd))
  {
    ld.IP[0] = ip[0];
    ld.IP[1] = ip[1];
    ld.IP[2] = ip[2];
    ld.IP[3] = ip[3];
    SendACK;
    return;
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;
}

void GetIP(void)
{
  SendACKonly;
  serial->println(ip);
}
