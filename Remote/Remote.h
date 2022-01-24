#pragma once

#include <Arduino.h>
#include <Ethernet.h>

#define SIGNATURE  0xAA55A5A5

// DIO line usage by Keyer Remote app
#define LED       0
#define KEYOUT    15
#define DIT       14
#define DAH       12
#define SK        4
#define RELAY     5
#define TONE      13
#define PB        16               // Connect pushbutton

typedef struct
{
  int16_t       Size;              // This data structures size in bytes
  char          Name[20];          // Application name
  int8_t        Rev;               // Holds application rev
  // WiFi parameters
  char          host[20];          // Host name (this system's name)
  char          ssid[30];          // WiFi SSID to connect with
  char          password[20];      // WiFi networks password
  int           Status;            // Connection status
  byte          servIP[4];         // Server IP address 
  int           tcpPort;
  int           udpPort;
  bool          APmode;            // True for access point
  // Keyer parameters
  int           wpm;               // keyer speed
  bool          STenable;          // Side tone enable
  int           STfreq;            // Side tone frequency
  bool          MuteEnable;        // External fred through audio mute
  int           MuteHold;          // Mute hold time in mS after key
  int           DDmode;            // If true then paddle uses high level command, dit and dah
  int           Signature;         // Must be 0xAA55A5A5 for valid data
} RemoteData;

extern RemoteData rd;

// Prototypes
void listNetworks(void);
void Software_Reset(void);
void SaveSettings(void);
void RestoreSettings(void);
void SetSrvIP(char *ip);
void GetSrvIP(void);
void Connect(void);
void Disconnect(void);
void Status(void);
void OpenClient(void);
void CloseClient(void);
void SendClientMessage(void);
void GetClientMessage(void);
