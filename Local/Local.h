#pragma once

#define SIGNATURE  0xAA55A5A5

typedef struct
{
  int16_t       Size;              // This data structures size in bytes
  char          Name[20];          // Application name
  int8_t        Rev;               // Holds application rev
  // Ethernet parameters
  byte          IP[4];             // IP address 
  int           tcpPort;
  int           udpPort;
  // Link test parameters
  bool          linkTesting;
  float         mean;              // Defined by remote system
  float         minError;
  float         maxError;
  float         sd;
  int           missedPackets;
  // Keyer parameters
  int           wpm;               // Code speed, words per minute
  int           Signature;         // Must be 0xAA55A5A5 for valid data
} LocalData;

extern LocalData ld;

// Prototypes
void Software_Reset(void);
void SaveSettings(void);
void RestoreSettings(void);
void SetPortDir(char *port, char *mode);
void SetPort(char *port, char *state);
void GetPort(int port);
void SetWPM(int wpm);
void String2upd(char *str);
void SetWPM(int wpm);
void String2upd(void);
void LinkStatus(void);
void ConnectStatus(void);
void SetIP(char *ipadd);
void GetIP(void);
