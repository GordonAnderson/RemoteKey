/*
 * Remote Controller firmware
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
 * The Remote controller uses an Adafruit Feather HUZZAH development board utilizing a ESP8266 WiFi module. 
 * This ESP8266 WiFi module incorporates a microcontroller clocked at 80 MHz. The Remote system reads and 
 * processes the contact closures from the attach paddle and key then sends this data using the WiFi link 
 * to the connect Local system. Below is a list of the Remote controller functions:
 * 
 *    - WiFi network connection 
 *    - Opens UDP and TCP ports with local controller
 *    - Built in iambic keyer
 *    - Sidetone generation
 *    - Ability to set key up and key down signals as well as higher level dit and dash signals
 *    - Mixes audio from computer with sidetone
 *    - Blanks computer audio during key operation
 *    - Connection status LED
 *    - Connection button to initiate link
 *    - USB powered
 *    - USB host interface commands to configure and save settings
 *
 * The Remote controller needs the SSID and password for the wireless access point you are using. You can even 
 * tether to a hotspot from a smartphone. After you are connected to the WiFi access point the local IP address 
 * is needed to make the connection. Most home internet links do not have a static IP but its pretty easy to 
 * get your IP and it will not change very often. Control of the station is done using PC remote control 
 * application and one easy way to get your local IP address is to open a browser and enter My IP in the 
 * search box. 
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
 *    - Add WPM command
 *    - Add enable / disable for side tone
 *    - Add enable / disable for audio mute
 *    - Add mute delay
 *    - Add connect button processing
 *
 * Release history:
 * 
 * Gordon Anderson
 * gaa@owt.com
 * 509.628.6851
 * 
 */
#include <arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <stdio.h>
#include <string.h>
#include <arduino-timer.h>
#include "Button.h"
#include "Remote.h"
#include "serial.h"
#include "Keyer.h"
#include "Errors.h"
#include <EEPROM.h>

extern "C" {
#include "user_interface.h"
}

auto timer = timer_create_default();

uint8_t       SequenceNr = 0;

RemoteData rd;

RemoteData Rev_1_rd = 
{
  sizeof(RemoteData),
  "Remote",
  1,
  // WiFi parameters
  "Keyer",
  "GAACE",
  "gaakg7yu",
  0,
  {10, 0, 0, 200},
  2015,
  2015,
  false,
  // Keyer parameters
  20,
  true,500,
  true,400,
  false,
  SIGNATURE
};

Button ConnectPin;

Keyer keyer;
uint32_t lastKDtime;

MDNSResponder mdns;

const char *Version = "Remote Keyer Version 1.0, November 27, 2015";

ESP8266WiFiClass wifi;

WiFiServer server(2015);
WiFiClient client;
WiFiUDP Udp;

bool OpenOnConnection = false;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet
char ReplyBuffer[UDP_TX_PACKET_MAX_SIZE];   // buffer to hold out going data

IPAddress serv(0,0,0,0);

// Connect LED timer function called every 500 mS.
// LED off with no connection.
// LED flashes when WiFi is connected.
// LED is on when client connection is open.
bool ConnectLED(void *)
{
  if(rd.Status == WL_CONNECTED)
  {
    if (client.connected())
    {
      digitalWrite(LED, LOW);
      return true;
    }
    if(digitalRead(LED)) digitalWrite(LED, LOW);
    else digitalWrite(LED, HIGH);
  }
  else digitalWrite(LED, HIGH);
  return true;
}

void SendDit(void)
{
  if(!rd.DDmode) return;
  if(client)
  {
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('.');
    Udp.write(SequenceNr);
    Udp.endPacket(); 
    Udp.flush();  
    delay(1);     
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('.');
    Udp.write(SequenceNr++);
    Udp.endPacket(); 
    Udp.flush();   
  }
  if(rd.MuteEnable)
  {
    digitalWrite(RELAY, HIGH);
    lastKDtime = millis();
  } 
}

void SendDah(void)
{
  if(!rd.DDmode) return;
  if(client)
  {
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('-');
    Udp.write(SequenceNr);
    Udp.endPacket(); 
    Udp.flush();  
    delay(1);     
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('-');
    Udp.write(SequenceNr++);
    Udp.endPacket(); 
    Udp.flush();   
  }
  if(rd.MuteEnable)
  {
    digitalWrite(RELAY, HIGH);
    lastKDtime = millis();
  }  
}

void KeyDown(void)
{
  if(client)
  {
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('D');
    Udp.write(SequenceNr);
    Udp.endPacket(); 
    Udp.flush();  
    delay(1);     
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('D');
    Udp.write(SequenceNr++);
    Udp.endPacket(); 
    Udp.flush();   
  }
  if(rd.MuteEnable)
  {
    digitalWrite(RELAY, HIGH);
    lastKDtime = millis();
  }
  digitalWrite(KEYOUT, HIGH);
}

void KeyUp(void)
{
  if(client)
  {
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('U');
    Udp.write(SequenceNr);
    Udp.endPacket(); 
    Udp.flush(); 
    delay(1);       
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('U');
    Udp.write(SequenceNr++);
    Udp.endPacket(); 
    Udp.flush(); 
  }  
  digitalWrite(KEYOUT, LOW);
}

bool ping(void *)
{
  // Send UDP message to keep link active, need this of iPhone WiFi link or else
  // after pause first element is truncated.
  if(client)
  {
    Udp.beginPacket(serv, rd.udpPort);
    Udp.write('p');
    Udp.write(SequenceNr);
    Udp.endPacket(); 
    Udp.flush(); 
  }  
  return true;
}

void setup()
{
  delay(100);
  // Storage for system data
  EEPROM.begin(sizeof(RemoteData));
  // Load the EEPROM save data into working memory
  EEPROM.get(0,rd);
  if(rd.Signature != SIGNATURE) rd = Rev_1_rd;
  // Init host interface
  SerialInit();
  // Init ditital IO
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pinMode(KEYOUT, OUTPUT);
  digitalWrite(KEYOUT, LOW);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  // Read server IP from data structure
  serv = rd.servIP;
  // Disconnet WiFi
  wifi.disconnect();
  // Setup keyer and setup callbacks
  keyer.begin();
  keyer.attachKeyDownCallBack(KeyDown);
  keyer.attachKeyUpCallBack(KeyUp);
  keyer.attachSendingDitCallBack(SendDit);
  keyer.attachSendingDahCallBack(SendDah);
  keyer.setSpeed(rd.wpm);
  keyer.enableSidetone(rd.STenable);
  keyer.setSidetoneFreq(rd.STfreq);
  // Start connect status LED
  timer.every(500, ConnectLED);
  // Start key alive link
  timer.every(1000, ping);
  ConnectPin.begin(PB);
}

// This function process all the serial IO and commands
void ProcessSerial(bool scan = true)
{
  // Put serial received characters in the input ring buffer
  if (Serial.available() > 0)
  {
    PutCh(Serial.read());
  }
  if (!scan) return;
  // If there is a command in the input ring buffer, process it!
  if (RB_Commands(&RB) > 0) while (ProcessCommand() == 0); // Process until flag that there is nothing to do
}

// Main processing loop.
void loop(void)
{
  timer.tick();
  ProcessSerial();
  keyer.process();
  rd.Status = wifi.status();
  if(rd.MuteEnable)
  {
    if((millis() - lastKDtime) > rd.MuteHold) digitalWrite(RELAY, LOW);
  }
  if(client.connected())
  {
    if(client.available() > 0)
    {
      serial->write(client.read());
    }
  }
  keyer.setSpeed(rd.wpm);
  keyer.setDDmode(rd.DDmode);
  keyer.enableSidetone(rd.STenable);
  keyer.setSidetoneFreq(rd.STfreq);
  if(ConnectPin.pressed())
  {
    // Here when connect button press is detected.
    // If we are connected then disconnect, if we are not connected then connect!
    if(rd.Status == WL_CONNECTED)
    {
       if (client.connected()) client.stop();
       wifi.disconnect();
    }
    else
    {
       wifi.begin(rd.ssid, rd.password);
       wifi.hostname(rd.host);
       OpenOnConnection = true;  
    }
  }
  if((OpenOnConnection) && (rd.Status == WL_CONNECTED))
  {
    OpenOnConnection = false;
    client.connect(serv, rd.tcpPort);
    Udp.begin(rd.udpPort);
  }
}

// Host commands, called from serial.cpp

void listNetworks()
{
  SendACKonly;
  // scan for nearby networks:
  serial->println("");
  serial->println("** Scan Networks **");
  int numSsid = wifi.scanNetworks();
  if (numSsid == -1) {
    serial->println("Couldn't get a wifi connection");
    while (true);
  }
  // print the list of networks seen:
  serial->print("number of available networks:");
  serial->println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    serial->print(thisNet);
    serial->print(") ");
    serial->print(wifi.SSID(thisNet));
    serial->print("\tSignal: ");
    serial->print(wifi.RSSI(thisNet));
    serial->print(" dBm");
    serial->print("\tEncryption: ");
    printEncryptionType(wifi.encryptionType(thisNet));
  }
}

void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      serial->println("WEP");
      break;
    case ENC_TYPE_TKIP:
      serial->println("WPA");
      break;
    case ENC_TYPE_CCMP:
      serial->println("WPA2");
      break;
    case ENC_TYPE_NONE:
      serial->println("None");
      break;
    case ENC_TYPE_AUTO:
      serial->println("Auto");
      break;
  }
}

void Software_Reset(void)
{
  system_restart();
}

void SaveSettings(void)
{
  rd.Signature = SIGNATURE;
  // replace values in byte-array cache with modified data
  // no changes made to flash, all in local byte-array cache
  EEPROM.put(0,rd);

  // actually write the content of byte-array cache to
  // hardware flash.  flash write occurs if and only if one or more byte
  // in byte-array cache has been changed, but if so, ALL 512 bytes are 
  // written to flash
  EEPROM.commit();
  SendACK;
}

void RestoreSettings(void)
{
  RemoteData rdata;

  // reload data for EEPROM, see the change
  EEPROM.get(0,rdata);
  if(rdata.Signature == SIGNATURE) rd = rdata;
  else
  {
    SetErrorCode(ERR_EEPROMWRITE);
    SendNAK;
    return;
  }
  SendACK;    
}

void SetSrvIP(char *ip)
{
  if(serv.fromString(ip))
  {
    SendACK;
    return;
  }
  SetErrorCode(ERR_BADARG);
  SendNAK;
}

void GetSrvIP(void)
{
  SendACKonly;
  serial->println(serv);
}

void Connect(void)
{
    wifi.begin(rd.ssid, rd.password);
    wifi.hostname(rd.host); 
    SendACK;
}

void Disconnect(void)
{
    if (wifi.status() != WL_CONNECTED)
    {
      SetErrorCode(ERR_WIFINOTCONNECTED);
      SendNAK;  
      return;
    }
    wifi.disconnect();  
    SendACK;
}

void Status(void)
{
    SendACKonly;
    switch (wifi.status())
    {
      case WL_CONNECTED:
        serial->println("Connected!");
        break;
      case WL_NO_SHIELD:
        serial->println("No shield!");
        break;
      case WL_IDLE_STATUS:
        serial->println("Idle!");
        break;
      case WL_NO_SSID_AVAIL:
        serial->println("No SSID!");
        break;
      case WL_SCAN_COMPLETED:
        serial->println("Complete!");
        break;
      case WL_CONNECT_FAILED:
        serial->println("Failed!");
        break;
      case WL_CONNECTION_LOST:
        serial->println("Lost!");
        break;
      case WL_DISCONNECTED:
        serial->println("Disconnected!");
        break;
      default:
        break;
    }
}

void mDNSresponder(void)
{
    if (!mdns.begin(rd.host, wifi.localIP(), 1))
    {
      Serial.println("Error setting up MDNS responder!");
    }
    else Serial.println("mDNS responder started");
    mdns.addService("mips", "tcp", 2015);
}

void AccessPoint(void)
{
    if(strlen(rd.password) == 0)
    {
      rd.APmode = true;
      wifi.hostname(rd.host);
      wifi.softAP((const char*)rd.ssid);
    }
    else
    {
      rd.APmode = true;
      wifi.hostname(rd.host);
      wifi.softAP((const char*)rd.ssid, (const char*)rd.password);
    }
}

void OpenClient(void)
{
  if(wifi.status() == WL_CONNECTED)
  {
    client.connect(serv, rd.tcpPort);
    Udp.begin(rd.udpPort);
    SendACK;
    return;
  }
  SetErrorCode(ERR_WIFINOTCONNECTED);
  SendNAK;  
}

void CloseClient(void)
{
  if (client.connected()) 
  {
    client.stop();
    SendACK;
    return;
  }
  SetErrorCode(ERR_CLIENTNOTCONNECTED);
  SendNAK;  
}

void SendClientMessage(void)
{
  char c;

  while((c=GetCh()) != 0xFF) if(c == ',') break;
  while((c=GetCh()) != 0xFF)
  {
    if(client.connected()) client.write(c);
    if(c == '\n') break;
  }
  SendACK;
}

void GetClientMessage(void)
{
  if(client.connected())
  {
    while (client.available() > 0)
    {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      // echo the bytes to the serial port:
      serial->write(thisChar);
    }
  }  
}
